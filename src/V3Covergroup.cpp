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

صنف Covergroup_سونپيو_Valid_Visitor final : عام VNVisitorConst {
    VMemberMap m_memberMap;
    std::map<ثابت AstVar*, ثابت Ast_ڳنڍ_FTask*>
        m_constructors;  // Implicit instance -> constructor
    std::سيٽ<ثابت AstVar*> m_mergeableTypeOptions;  // See mergeableTypeOptions()
    ثابت Ast_ڳنڍ_FTask* m_ftaskp = خالي_اشارو;
    منطقي m_collecting = سچ;
    منطقي m_valid = سچ;

    فراغ گهمو(AstClass* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = خالي_اشارو;
        إذا (m_collecting) {
            ثابت Ast_ڳنڍ_FTask* ثابت constructorp
                = VN_CAST(m_memberMap.ڳوليو_Member(nodep, "new"), ڳنڍ_FTask);
            لكل (ثابت Ast_ڳنڍ* itemp = nodep->membersp(); itemp; itemp = itemp->nextp()) {
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
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_سونپيو* nodep) override {
        إذا (!m_collecting) {
            ثابت AstVar* varp = خالي_اشارو;
            إذا (ثابت Ast_ڳنڍ_Var_Ref* ثابت refp = VN_CAST(nodep->lhsp(), ڳنڍ_Var_Ref)) {
                varp = refp->varp();
            } وإلا إذا (ثابت AstMemberSel* ثابت selp = VN_CAST(nodep->lhsp(), MemberSel)) {
                varp = selp->varp();
            }
            ثابت خودڪار it = m_constructors.ڳوليو(varp);
            إذا (it != m_constructors.پڇاڙي() && (!m_ftaskp || m_ftaskp != it->ٻيو)) {
                m_valid = خطأ;
                nodep->v3error("Embedded covergroup variable "
                               << varp->prettyNameQ()
                               << " may only be assigned in the enclosing class's 'new' method "
                                  "(IEEE 1800-2023 19.4).");
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ_Var_Ref* nodep) override {
        // Type options may be set at any time (IEEE 1800-2023 19.7.1), so SystemVerilog may set
        // type_option.merge_instances with any use of a type_option but a read of another
        // member, as not every write is marked yet (std::randomize)
        إذا (m_collecting && nodep->varp()->name() == "type_option") {
            ثابت AstStructSel* ثابت selp = VN_CAST(nodep->backp(), StructSel);
            إذا (!selp || selp->fromp() != nodep || selp->name() == "merge_instances"
                || !nodep->access().isReadOnly()) {
                m_mergeableTypeOptions.وجهو(nodep->varp());
            }
        }
        iterateChildrenConst(nodep);
    }
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }

عام:
    واضح Covergroup_سونپيو_Valid_Visitor(AstNetlist* nodep) {
        // Uses can precede their enclosing class in the tree.
        iterateConst(nodep);
        m_collecting = خطأ;
        إذا (!m_constructors.خالي_جايون()) iterateConst(nodep);
    }
    منطقي valid() ثابت { أرجع m_valid; }
    // type_option variables through which SystemVerilog may set type_option.merge_instances, so
    // that covergroups are tested with optionVar(true)
    ثابت std::سيٽ<ثابت AstVar*>& mergeableTypeOptions() ثابت { أرجع m_mergeableTypeOptions; }
};

//######################################################################
// Covergroup expression validation visitor

صنف Covergroup_اظهار_Valid_Visitor final : عام VNVisitor {
    ثابت std::سيٽ<ثابت AstVar*>& m_sampleMembers;
    ثابت std::سيٽ<ثابت AstVar*>& m_constructorRefMembers;
    منطقي m_اندريون_Coverage_اظهار = خطأ;
    منطقي m_sampleFormalAllowed = خطأ;

    فراغ جاچ_Sample_اظهار(Ast_ڳنڍ* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_sampleFormalAllowed);
        m_sampleFormalAllowed = سچ;
        iterate_And_ايندڙ_Null(nodep);
    }

    فراغ جاچ_Coverage_اظهار(Ast_ڳنڍ* nodep) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_اندريون_Coverage_اظهار);
        m_اندريون_Coverage_اظهار = سچ;
        iterate_And_ايندڙ_Null(nodep);
    }

    فراغ گهمو(AstCoverpoint* nodep) override {
        جاچ_Sample_اظهار(nodep->exprp());
        جاچ_Sample_اظهار(nodep->iffp());
        iterate_And_ايندڙ_Null(nodep->binsp());
        iterate_And_ايندڙ_Null(nodep->optionsp());
    }
    فراغ گهمو(AstCoverCross* nodep) override {
        iterate_And_ايندڙ_Null(nodep->itemsp());
        جاچ_Sample_اظهار(nodep->iffp());
        iterate_And_ايندڙ_Null(nodep->optionsp());
        iterate_And_ايندڙ_Null(nodep->binsp());
    }
    فراغ گهمو(AstCoverCrossBin* nodep) override {
        iterate_And_ايندڙ_Null(nodep->selectp());
        جاچ_Sample_اظهار(nodep->iffp());
    }
    فراغ گهمو(AstCoverCrossWith* nodep) override {
        iterate_And_ايندڙ_Null(nodep->selectp());
        جاچ_Coverage_اظهار(nodep->filterp());
        جاچ_Coverage_اظهار(nodep->countp());
    }
    فراغ گهمو(AstCoverBinsof* nodep) override { جاچ_Coverage_اظهار(nodep->rangesp()); }
    فراغ گهمو(AstCoverBin* nodep) override {
        جاچ_Coverage_اظهار(nodep->rangesp());
        جاچ_Sample_اظهار(nodep->iffp());
        جاچ_Coverage_اظهار(nodep->arraySizep());
        جاچ_Coverage_اظهار(nodep->transp());
    }
    فراغ گهمو(AstVarRef* nodep) override {
        إذا (!m_sampleFormalAllowed && m_sampleMembers.ڳڻپ(nodep->varp())) {
            nodep->v3error("Covergroup sample formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may only be used in a coverpoint or conditional guard "
                              "expression (IEEE 1800-2023 19.8.1).");
        }
        إذا (m_اندريون_Coverage_اظهار && m_constructorRefMembers.ڳڻپ(nodep->varp())) {
            nodep->v3error("Ref covergroup constructor formal argument "
                           << nodep->varp()->prettyNameQ()
                           << " may not be used in a covergroup expression "
                              "(IEEE 1800-2023 19.5).");
        }
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        إذا (m_اندريون_Coverage_اظهار && nodep->taskp()) {
            منطقي invalidDirection = خطأ;
            لكل (Ast_ڳنڍ* stmtp = nodep->taskp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
                إذا (varp && varp->isIO() && varp->isWritable()) {
                    invalidDirection = سچ;
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
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    Covergroup_اظهار_Valid_Visitor(ثابت std::سيٽ<ثابت AstVar*>& sampleMembers,
                               ثابت std::سيٽ<ثابت AstVar*>& constructorRefMembers)
        : m_sampleMembers{sampleMembers}
        , m_constructorRefMembers{constructorRefMembers} {}
    فراغ جاچ(Ast_ڳنڍ* nodep) { iterate(nodep); }
};

//######################################################################
// Bins of one declaration whose values are computed rather than listed: bin k covers
// [m_lo + k * m_stride, m_lo + (k + 1) * m_stride - 1], and the last bin extends to m_hi.  An
// array bin element is a run of single-value bins; automatic bins partition the coverpoint
// domain.  Bounds are coverpoint values at FunctionalCoverageVisitor::runWidth(),
// sign-extended like a CrossValueRange's.

صنف Bin_لاڳيتو final {
عام:
    // MEMBERS
    uint32_t m_ڳڻپ;  // Number of bins
    V3_عدد m_lo;  // Lowest value of the first bin
    V3_عدد m_stride;  // Number of values of each bin but the last
    V3_عدد m_hi;  // Highest value of the last bin
    منطقي m_خالي_جايون = خطأ;  // A single bin without a value of the coverpoint type
    uint32_t m_declared = 0;  // Runtime index of the first bin, once generated

    // CONSTRUCTORS
    Bin_لاڳيتو(Ast_ڳنڍ* nodep, صحيح ويڪر, uint32_t ڳڻپ)
        : m_ڳڻپ{ڳڻپ}
        , m_lo{nodep, ويڪر}
        , m_stride{nodep, ويڪر, 1}
        , m_hi{nodep, ويڪر} {}
};

//######################################################################
// Functional coverage visitor

صنف FunctionalCoverageVisitor final : عام VNVisitor {
    // NODE STATE
    // Entire netlist:
    //  AstCoverpoint::user1p()  -> AstVar*.  Previous-value variable for transition bins
    //  AstCoverpoint::user2()   -> bool.  Had a bins declaration ignored, so no automatic bins
    ثابت VNUser1_اندريون_Use m_inuser1;
    ثابت VNUser2_اندريون_Use m_inuser2;

    // STATE
    std::سيٽ<AstCoverpoint*>
        m_runtimePoints;  // Points needing value metadata and live-bin mapping
    std::سيٽ<AstCoverCross*> m_runtimeCrosses;  // Crosses over finalized live-bin dimensions
    std::map<AstVar*, AstVar*> m_excludedVars;  // Sample-time state-exclusion flags
    AstClass* m_covergroupp = خالي_اشارو;  // Current covergroup being processed
    Ast_ڳنڍ_Module* m_unitp = خالي_اشارو;  // Design unit declaring the current class, outside classes
    AstClass* m_enclosingClassp = خالي_اشارو;  // Class lexically enclosing the covergroup, if any
    AstVar* m_embeddedVarp = خالي_اشارو;  // Embedded covergroup member of m_enclosingClassp, if any
    std::string m_covergroupName;  // Current covergroup's type name, see covergroupTypeName()
    AstFunc* m_sampleFuncp = خالي_اشارو;  // Current sample() function
    AstIf* m_cpGuardp = خالي_اشارو;  // Current coverpoint's iff guard, holding its sampling
    AstFunc* m_constructorp = خالي_اشارو;  // Current constructor
    std::ويڪٽر<AstCoverpoint*> m_coverpoints;  // Coverpoints in current covergroup
    std::map<std::string, AstCoverpoint*> m_coverpointMap;  // Name -> coverpoint for fast lookup
    std::ويڪٽر<AstCoverCross*> m_coverCrosses;  // Cross coverage items in current covergroup
    std::ويڪٽر<Ast_Cg_Option_سونپيو*> m_cgOptions;  // Covergroup-level options, before lowering
    uint32_t m_cg_Type_وزن = 1;  // The covergroup's type_option.weight, a constant
    منطقي m_cg_ملايو_Instances = خطأ;  // The covergroup's type_option.merge_instances, a constant
    // The covergroup's type_option.merge_instances is, or SystemVerilog may make it, true
    منطقي m_cg_May_ملايو = خطأ;
    // type_option members through which SystemVerilog may set type_option.merge_instances,
    // see CovergroupAssignValidVisitor::mergeableTypeOptions()
    ثابت std::سيٽ<ثابت AstVar*>& m_mergeableTypeOptions;
    صحيح m_cg_At_گهٽ_کان_گهٽ = 1;  // Covergroup option.at_least

    بنية EmbeddedEventTrigger final {
        File_سٽ* eventFl;  // Clocking-event source location
        AstVar* بنياد_Varp;  // Base enclosing-class member in the event expression
        AstVar* memberVarp;  // Selected member in a 'base.member' expression, or nullptr
        VEdgeType ڪنارو_Type;  // Clocking-event edge qualifier
        AstVar* prevVarp;  // Member containing the previous event value, or nullptr
        EmbeddedEventTrigger(File_سٽ* eventFl, AstVar* بنياد_Varp, AstVar* memberVarp,
                             VEdgeType ڪنارو_Type)
            : eventFl{eventFl}
            , بنياد_Varp{بنياد_Varp}
            , memberVarp{memberVarp}
            , ڪنارو_Type{ڪنارو_Type}
            , prevVarp{خالي_اشارو} {}
    };

    std::سيٽ<std::string> m_crossed_Cp_نالا;  // Coverpoints referenced by a cross
    std::map<std::string, AstVar*> m_cpVarMap;  // Coverpoint name -> its VlCoverpoint member
    بنية BinRuns final {
        std::ويڪٽر<Bin_لاڳيتو> runs;  // Runs of an array or automatic bins declaration, in order
        uint32_t ڳڻپ = 0;  // Bins across all runs
        منطقي unsupported = خطأ;  // Too many bins, or invalid: the declaration is ignored
        // The value of each bin, which names it, of a wildcard array; else the bins are indexed
        std::ويڪٽر<std::string> قدرون;
        // Too many values of an ignore or illegal wildcard array, which is then one bin
        منطقي single = خطأ;
    };
    بنية Cross_Bin_قدرون final {
        AstCoverBin* binp;  // Declaration owning this Normal bin
        Ast_ڳنڍ_اظهار* valuep;  // Individual array-bin value, or nullptr for a scalar bin
        ثابت Bin_لاڳيتو* runp = خالي_اشارو;  // Run computing the bin's values, if any
        uint32_t element = 0;  // Index of the bin within runp
    };
    بنية Bin_وسعت final {
        uint32_t پهريون = 0;  // First Normal index of the bin declaration
        uint32_t ڳڻپ = 0;  // Number of Normal bins of the declaration
        uint32_t declared = 0;  // First runtime bin index, across all bin kinds
        int32_t sized = -1;  // Index of the sized array, placed at construction; or -1
    };
    بنية CoverpointBins final {
        uint32_t ڪل = 0;  // Number of Normal bins
        Ast_ڳنڍ_اظهار* exprp = خالي_اشارو;  // Sampled expression, for the value domain
        منطقي crossed = خطأ;  // Feeds a cross, which needs 'values'
        std::ويڪٽر<Cross_Bin_قدرون> قدرون;  // Values in runtime Normal-bin index order
        std::unordered_map<std::string, Bin_وسعت> spans;  // Declared bin name -> index span
        Bin_وسعت implicitAuto{0, 0, 0};  // Implicit automatic bins, each named 'auto_<i>'
        std::deque<Bin_لاڳيتو> runs;  // Runs 'values' refers to
    };
    std::map<AstVar*, CoverpointBins> m_cpBins;  // Runtime coverpoint -> binsof index ranges
    // Names of the bins declarations each coverpoint ignored, which binsof selects as no bins
    std::map<ثابت AstCoverpoint*, std::ويڪٽر<std::string>> m_droppedBins;
    // Prefixes of the constructor temporaries of constructed bins, as coverpoint and bin names
    // alone may repeat: coverpoint 'a_' bins 'b', and coverpoint 'a' bins '_b'
    V3_Unique_نالا m_sized_نالا{"__Vsized"};
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> m_detached_قدرون;  // Array-bin values m_cpBins refers to
    std::سيٽ<AstCoverCross*>
        m_droppedCrosses;  // Crosses with a bare-variable item: drop (COVERIGN)
    std::map<uint32_t, AstCoverpointDType*> m_cpDTypes;  // Hit-list bound -> interned dtype
    استعمال CrossShape = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, uint64_t, منطقي>;
    std::map<CrossShape, AstCoverCrossDType*> m_cxDTypes;
    AstVar* m_cgInstVarp = خالي_اشارو;  // __Vcg_inst handle member of the current covergroup

    VMemberMap m_memberMap;  // Member names cached for fast lookup

    // METHODS
    // The covergroup's 'option' or static 'type_option' member (V3LinkParse creates both)
    AstVar* optionVar(منطقي typeOption) {
        AstVar* ثابت varp = VN_AS(
            m_memberMap.ڳوليو_Member(m_covergroupp, typeOption ? "type_option" : "option"), Var);
        UASSERT_OBJ(varp, m_covergroupp, "Covergroup missing option member");
        أرجع varp;
    }

    // 'option.<member>' or 'type_option.<member>', per optionVarp
    AstStructSel* newOptionSel(File_سٽ* fl, AstVar* optionVarp, ثابت std::string& member,
                               VAccess access) {
        ثابت AstMemberDType* ثابت memberp
            = VN_AS(m_memberMap.ڳوليو_Member(optionVarp->dtypep()->skipRefp(), member), MemberDType);
        UASSERT_OBJ(memberp, optionVarp, "Coverage option structure missing '" << member << "'");
        Ast_ڳنڍ_اظهار* ثابت fromp = optionVarp->lifetime().isStatic()
                                       ? جديد AstVarRef{fl, optionVarp, access}
                                       : memberRef(fl, optionVarp, access);
        AstStructSel* ثابت selp = جديد AstStructSel{fl, fromp, member};
        selp->dtypep(memberp->subDTypep()->skip_Ref_منزل_Enump());
        selp->did_ويڪر(سچ);
        أرجع selp;
    }

    // A constant option.at_least; a negative count of hits has no meaning, so takes the default
    ساكن صحيح const_At_گهٽ_کان_گهٽ(ثابت AstConst* constp) {
        ثابت صحيح قدر = constp->منزل_SInt();
        إذا (قدر >= 0) أرجع قدر;
        constp->v3warn(COVERIGN, "Ignoring negative 'option.at_least'; using 1");
        أرجع 1;
    }

    // An option's constant value, or nullptr after a warning, as non-constant is unsupported
    ساكن ثابت AstConst* constOption(ثابت Ast_ڳنڍ* valuep, VCoverOptionType optType) {
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) أرجع constp;
        valuep->v3warn(COVERIGN, "Ignoring unsupported: non-constant 'option."
                                     << optType.ascii() << "'; using default value");
        أرجع خالي_اشارو;
    }

    // Store the covergroup-level options (IEEE 1800-2023 19.7) where SystemVerilog and the
    // runtime read them.  The instance options, option.weight and option.get_inst_coverage,
    // are evaluated by the constructor, as are the other instance options; the type options,
    // type_option.weight and type_option.merge_instances, are constant, and initialize the
    // static member.  Without its own type_option.merge_instances, a covergroup has the
    // default of --coverage-merge-instances.  A constant option.at_least is the default of
    // the coverpoints and crosses.
    فراغ هيٺاهون_Covergroup_Options() {
        منطقي ملايو_سيٽ = خطأ;  // The covergroup sets type_option.merge_instances
        m_cg_At_گهٽ_کان_گهٽ = 1;
        لكل (Ast_Cg_Option_سونپيو* ثابت optp : m_cgOptions) {
            إذا (optp->optType() == VCoverOptionType::AT_گهٽ_کان_گهٽ) {
                إذا (ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType())) {
                    m_cg_At_گهٽ_کان_گهٽ = const_At_گهٽ_کان_گهٽ(constp);
                }
                VL_DO_DANGLING(ڌڪو_Deletep(optp->unlink_Fr_پٺ()), optp);
                استمر;
            }
            File_سٽ* ثابت fl = optp->fileline();
            // V3Width left the type options constant, and type_option.weight non-negative
            ثابت AstConst* ثابت constp = VN_CAST(optp->valuep(), Const);
            std::string member = "weight";
            إذا (optp->optType() == VCoverOptionType::ملايو_INSTANCES) {
                member = "merge_instances";
                m_cg_ملايو_Instances = !constp->انگ().isEqZero();
                ملايو_سيٽ = سچ;
            } وإلا إذا (optp->optType() == VCoverOptionType::GET_INST_COVERAGE) {
                member = "get_inst_coverage";
            } وإلا {
                UASSERT_OBJ(optp->optType() == VCoverOptionType::وزن, optp,
                            "Unexpected covergroup option reaching V3Covergroup");
                إذا (optp->typeOption()) m_cg_Type_وزن = constp->منزل_UInt();
            }
            Ast_سونپيو* ثابت assignp = جديد Ast_سونپيو{
                fl, newOptionSel(fl, optionVar(optp->typeOption()), member, VAccess::WRITE),
                optp->valuep()->unlink_Fr_پٺ()};
            إذا (optp->typeOption()) {
                m_covergroupp->جوڙيو_Membersp(جديد AstInitialStatic{fl, assignp});
                VL_DO_DANGLING(ڌڪو_Deletep(optp->unlink_Fr_پٺ()), optp);
            } وإلا {
                optp->replaceWith(assignp);
                VL_DO_DANGLING(ڌڪو_Deletep(optp), optp);
            }
        }
        m_cgOptions.clear();
        إذا (!ملايو_سيٽ && v3Global.opt.coverage_ملايو_Instances()) {
            File_سٽ* ثابت fl = m_covergroupp->fileline();
            m_covergroupp->جوڙيو_Membersp(جديد AstInitialStatic{
                fl, جديد Ast_سونپيو{
                        fl, newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::WRITE),
                        جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}}});
            m_cg_ملايو_Instances = سچ;
        }
        m_cg_May_ملايو = m_cg_ملايو_Instances || m_mergeableTypeOptions.ڳڻپ(optionVar(سچ));
    }

    // The weight of an item in the coverage database, which merges the instances: its
    // type_option.weight if the covergroup merges them too (IEEE 1800-2023 19.11.3); else its
    // option.weight if a constant, and so of every instance; else its type_option.weight
    uint32_t شي_Database_وزن(Ast_ڳنڍ* optionsp) ثابت {
        ثابت Ast_ڳنڍ_اظهار* weightp = خالي_اشارو;  // The option.weight in effect
        uint32_t type_وزن = 1;
        لكل (Ast_ڳنڍ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::وزن)) استمر;
            // V3Width left type_option.weight a non-negative constant
            إذا (optp->typeOption()) {
                type_وزن = VN_AS(optp->valuep(), Const)->منزل_UInt();
            } وإلا {
                weightp = optp->valuep();
            }
        }
        إذا (m_cg_ملايو_Instances) أرجع type_وزن;
        إذا (!weightp) أرجع 1;
        إذا (ثابت AstConst* ثابت constp = VN_CAST(weightp, Const)) أرجع constp->منزل_UInt();
        أرجع type_وزن;
    }

    // Configure an item's option.weight, its weight in instance coverage (IEEE 1800-2023
    // 19.11), and, if the covergroup may merge its instances, its type_option.weight, its
    // weight in type coverage then (19.11.3).
    فراغ generate_شي_وزن(File_سٽ* fl, AstVar* شي_Varp, Ast_ڳنڍ* optionsp) {
        لكل (Ast_ڳنڍ* nodep = optionsp; nodep; nodep = nodep->nextp()) {
            ثابت AstCoverOption* ثابت optp = VN_AS(nodep, CoverOption);
            إذا (!(optp->optType() == VCoverOptionType::وزن)) استمر;
            إذا (!optp->typeOption()) {
                m_constructorp->جوڙيو_Stmtsp(
                    شي_Call(fl, شي_Varp, VCMethod::COVERGROUP_وزن,
                             {optp->valuep()->clone_وڻ(خطأ), file_سٽ_Debug(optp->fileline())})
                        ->makeStmt());
            } وإلا إذا (m_cg_May_ملايو) {
                // V3Width left type_option.weight a non-negative constant
                m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, شي_Varp, VCMethod::COVERGROUP_TYPE_وزن,
                                                   {optp->valuep()->clone_وڻ(خطأ)})
                                              ->makeStmt());
            }
        }
    }

    فراغ processCovergroup() {
        UINFO(4, "Processing covergroup: " << m_covergroupp->name() << " with "
                                           << m_coverpoints.سائيز() << " coverpoints and "
                                           << m_coverCrosses.سائيز() << " crosses");

        m_crossed_Cp_نالا.clear();
        m_cpVarMap.clear();
        m_cpBins.clear();
        m_droppedBins.clear();
        m_sized_نالا.reset();
        m_runtimePoints.clear();
        m_runtimeCrosses.clear();
        m_excludedVars.clear();
        m_droppedCrosses.clear();
        m_cgInstVarp = خالي_اشارو;
        m_cg_Type_وزن = 1;
        m_cg_ملايو_Instances = خطأ;
        m_cg_May_ملايو = خطأ;

        هيٺاهون_Covergroup_Options();

        // Scan every cross item to record the coverpoints it references (the cross dimensions)
        // and to flag any cross naming a bare variable -- a would-be implicit coverpoint, which
        // Verilator does not synthesize.  An unresolvable item drops only that one cross (with a
        // COVERIGN in generateCrossCode), leaving the rest of the covergroup intact.
        لكل (AstCoverCross* crossp : m_coverCrosses) {
            لكل (Ast_ڳنڍ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;  // hierarchical ref: dropped in generateCrossCode
                إذا (m_coverpointMap.ڳوليو(refp->name()) == m_coverpointMap.پڇاڙي()) {
                    m_droppedCrosses.وجهو(crossp);  // bare variable: drop this cross only
                } وإلا {
                    m_crossed_Cp_نالا.وجهو(refp->name());
                }
            }
        }

        std::ويڪٽر<Ast_ڳنڍ*> pending;
        std::map<AstCoverpoint*, std::ويڪٽر<AstCoverCross*>> consumers;
        std::map<AstCoverCross*, std::ويڪٽر<AstCoverpoint*>> inputs;
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            check_Bin_نالا(cpp);
            checkConstructedBins(cpp);
            إذا (!cpp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            // Bins without values leave the report (IEEE 1800-2023 19.11.1), exclusions or not.
            // Constructed bins get their values when the covergroup is constructed.
            إذا (!coverpointHasStateExclusions(cpp) && !coverpoint_Has_خالي_جايون_Bins(cpp)
                && !coverpointHasConstructedBins(cpp)) {
                استمر;
            }
            m_runtimePoints.وجهو(cpp);
            pending.ڌڪو_پٺ(cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            إذا (m_droppedCrosses.ڳڻپ(crossp)) استمر;
            لكل (Ast_ڳنڍ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (refp->exprp()) استمر;
                ثابت خودڪار نقطو = m_coverpointMap.ڳوليو(refp->name());
                إذا (نقطو != m_coverpointMap.پڇاڙي()) {
                    consumers[نقطو->ٻيو].ڌڪو_پٺ(crossp);
                    inputs[crossp].ڌڪو_پٺ(نقطو->ٻيو);
                }
            }
            // The values of a filter's bin tuples are known when the covergroup is constructed
            إذا (crossp->exists([](ثابت AstCoverCrossWith*) { أرجع سچ; })) {
                m_runtimeCrosses.emplace(crossp);
                pending.ڌڪو_پٺ(crossp);
            }
        }
        لكل (سائيز_t ايندڙ = 0; ايندڙ < pending.سائيز(); ++ايندڙ) {
            إذا (AstCoverpoint* ثابت pointp = VN_CAST(pending[ايندڙ], Coverpoint)) {
                لكل (AstCoverCross* ثابت crossp : consumers[pointp]) {
                    إذا (m_runtimeCrosses.emplace(crossp).ٻيو) pending.ڌڪو_پٺ(crossp);
                }
            } وإلا {
                لكل (AstCoverpoint* ثابت pointp : inputs[VN_AS(pending[ايندڙ], CoverCross)]) {
                    إذا (pointp->exprp()->dtypep()->skipRefp()->isIntegralOrPacked()
                        && m_runtimePoints.emplace(pointp).ٻيو) {
                        pending.ڌڪو_پٺ(pointp);
                    }
                }
            }
        }

        // The instance node owns this instance's coverpoint/cross runtimes, so it must exist
        // before any of them is created.  Emitted first, ahead of both generate loops.
        generateInstanceAttach();

        // For each coverpoint, generate sampling code
        لكل (AstCoverpoint* cpp : m_coverpoints) generate_Coverpoint_ڪوڊ(cpp);

        // For each cross, generate sampling code
        لكل (AstCoverCross* crossp : m_coverCrosses) generate_Cross_ڪوڊ(crossp);

        // Every cross has been built, so runtime points only need their exclusions from here.
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            إذا (!m_runtimePoints.ڳڻپ(cpp)) استمر;
            m_constructorp->جوڙيو_Stmtsp(شي_Call(cpp->fileline(), m_cpVarMap.at(cpp->name()),
                                               VCMethod::COVERGROUP_قدر_RELEASE)
                                          ->makeStmt());
        }
        لكل (Ast_ڳنڍ_اظهار* valuep : m_detached_قدرون) VL_DO_DANGLING(ڌڪو_Deletep(valuep), valuep);
        m_detached_قدرون.clear();

        // Generate coverage computation code (even for empty covergroups).  Bin registration
        // with the coverage database is handled per coverpoint/cross by their runtime
        // registerBins() calls (emitted in generateCoverpoint/generateCross).
        generate_Coverage_Computation_ڪوڊ();
    }

    ساكن constexpr سائيز_t قدر_LIST_ENTRIES = 256;  // Metadata entries per constructor call

    // The value of a constant count, of bins or value tuples: -1 if it is negative, and
    // saturated above the largest limit
    ساكن int64_t const_ڳڻپ(ثابت AstConst* constp) {
        ثابت V3_عدد& انگ = constp->انگ();
        إذا (constp->isSigned() && انگ.isNegative()) أرجع -1;
        أرجع انگ.most_سيٽ_ٻٽو_انگ_P1() > 32 ? INT64_MAX : static_cast<int64_t>(انگ.منزل_UQuad());
    }

    // Whether a count, 'whatp', is integral and, if constant, positive, as the standard's clause
    // 'clausep' requires; else errors
    ساكن منطقي check_ڳڻپ(ثابت Ast_ڳنڍ_اظهار* countp, ثابت حرف* whatp, ثابت حرف* clausep) {
        ثابت AstConst* ثابت constp = VN_CAST(countp, Const);
        إذا (!countp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            countp->v3error(whatp << " must be integral (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        } وإلا إذا (constp && (constp->انگ().isFourState() || const_ڳڻپ(constp) < 1)) {
            countp->v3error(whatp << " must be >= 1, got "
                                  << (constp->انگ().isFourState() ? constp->انگ().ascii(خطأ)
                                                                  : constp->انگ().منزل_Decimal_S())
                                  << " (IEEE 1800-2023 " << clausep << ")");
            أرجع خطأ;
        }
        أرجع سچ;
    }

    // The number of bins requested by a valid 'bins auto[N]', or 0
    ساكن uint32_t autoBinsRequested(ثابت AstCoverBin* binp) {
        ثابت AstConst* ثابت constp = VN_CAST(binp->arraySizep(), Const);
        إذا (!constp) أرجع 0;
        ثابت int64_t ڳڻپ = const_ڳڻپ(constp);
        أرجع ڳڻپ < 1 || ڳڻپ > v3Global.opt.coverageMaxBins() ? 0
                                                                   : static_cast<uint32_t>(ڳڻپ);
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
    ساكن Ast_ڳنڍ* binRangesp(ثابت AstCoverBin* binp) {
        ثابت AstCoverWith* ثابت withp = binWith(binp);
        إذا (!withp) أرجع binp->rangesp();
        أرجع VN_IS(withp->subp(), CoverpointRef) ? خالي_اشارو : withp->subp();
    }

    // Report and delete a bins declaration of a name that another of its coverpoint has
    فراغ check_Bin_نالا(AstCoverpoint* coverpointp) {
        std::سيٽ<std::string> نالا;
        لكل (Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (نالا.emplace(binp->name()).ٻيو) استمر;
            binp->v3error("Duplicate bin " << binp->prettyNameQ() << " in coverpoint "
                                           << coverpointp->prettyNameQ()
                                           << " (IEEE 1800-2023 3.13)");
            VL_DO_DANGLING(ڌڪو_Deletep(binp->unlink_Fr_پٺ()), binp);
        }
    }

    // Delete an ignored bins declaration, which binsof then selects as no bins
    فراغ dropBins(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp) {
        m_droppedBins[coverpointp].ڌڪو_پٺ(binp->name());
        VL_DO_DANGLING(ڌڪو_Deletep(binp->unlink_Fr_پٺ()), binp);
    }

    // Check the size of a sized array of bins, which drops an invalid array.  A real coverpoint's
    // are unsupported, and treated as arrays of a bin per value.  False unless it stays sized.
    منطقي check_Bins_Array_سائيز(ثابت AstCoverpoint* coverpointp, AstCoverBin* binp, منطقي integral) {
        Ast_ڳنڍ_اظهار* ثابت sizep = binp->arraySizep();
        إذا (VN_IS(sizep, Unbounded)) {  // A parameter of '$'; see bins_orBraE
            binp->v3error("Bins array size must be integral, not '$' (IEEE 1800-2023 19.5.1)");
        } وإلا إذا (check_ڳڻپ(sizep, "Bins array size", "19.5.1")) {
            إذا (integral) أرجع سچ;
            binp->v3warn(COVERIGN, "Unsupported: 'bins' explicit array size of a real "
                                   "coverpoint (treated as '[]')");
            VL_DO_DANGLING(ڌڪو_Deletep(sizep->unlink_Fr_پٺ()), sizep);
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
        لكل (Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep;) {
            AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            nodep = nodep->nextp();
            إذا (!isConstructedBins(binp)) استمر;
            إذا (isSizedArray(binp) && !check_Bins_Array_سائيز(coverpointp, binp, integral)) استمر;
            إذا (!binp->isWildcard()
                || sizedWildcardRuns(binp, coverpointp->exprp())
                       <= v3Global.opt.coverageMaxBins()) {
                إذا (binWith(binp) && withCandidatesOver(binp, coverpointp->exprp())) {
                    binp->v3warn(COVERIGN, "Unsupported: 'with' filter of more than 2**32 "
                                           "candidate values; bin "
                                               << binp->prettyNameQ() << " ignored");
                    إذا (binp->binsType().bin_Is_عام()) coverpointp->user2(سچ);
                    dropBins(coverpointp, binp);
                }
                استمر;
            }
            // An ignore or illegal array still excludes or checks its values, as one bin
            ثابت منطقي single = !binp->binsType().bin_Is_عام() && !binWith(binp);
            binp->v3warn(COVERIGN,
                         "Unsupported: " << (binWith(binp) ? "'with' filter of wildcard '"
                                                           : "sized wildcard array '")
                                         << binp->binsType().verilogKwd()
                                         << "' of more than --coverage-max-bins of "
                                         << v3Global.opt.coverageMaxBins()
                                         << " ranges of values; bin " << binp->prettyNameQ()
                                         << (single ? " treated as one bin" : " ignored") << "\n"
                                         << binp->warn_وڌيڪ()
                                         << "... Suggest a larger --coverage-max-bins");
            إذا (single) {
                Ast_ڳنڍ_اظهار* ثابت sizep = binp->arraySizep();
                VL_DO_DANGLING(ڌڪو_Deletep(sizep->unlink_Fr_پٺ()), sizep);
                binp->isArray(خطأ);
                استمر;
            }
            إذا (binp->binsType().bin_Is_عام()) coverpointp->user2(سچ);
            dropBins(coverpointp, binp);
        }
    }

    // True if a 'with' filter would be evaluated for more than 2**32 candidate values, known
    // now for the coverpoint's name, or for a range list of constants: each is evaluated once
    // but for an array 'b[N]', which keeps their order and duplicates (see withBegin())
    ساكن منطقي withCandidatesOver(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp) {
        ثابت صحيح ويڪر = لاڳيتو_ويڪر(exprp);
        std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> runs;
        إذا (!binRangesp(binp)) runs = coverpoint_قدرون(binp, exprp);
        ثابت خودڪار constant
            = [](ثابت Ast_ڳنڍ* nodep) { أرجع VN_IS(nodep, Const) || VN_IS(nodep, Unbounded); };
        لكل (Ast_ڳنڍ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            // Else known when constructed
            إذا (irp ? !constant(irp->lhsp()) || !constant(irp->rhsp()) : !VN_IS(rangep, Const)) {
                أرجع خطأ;
            }
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)
                || cross_Range_خالي_جايون(range)) {
                استمر;
            }
            std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> مليو{{range.lo, range.hi}};
            إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                مليو.clear();
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), مليو);
            }
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : مليو) {
                runs.emplace_پٺ(V3_عدد{rangep, ويڪر, لاڳيتو.پهريون},
                                  V3_عدد{rangep, ويڪر, لاڳيتو.ٻيو});
            }
        }
        إذا (!isSizedArray(binp)) {  // The union of the values
            std::ترتيب_ڏيو(runs.begin(), runs.پڇاڙي(), [](ثابت خودڪار& lhs, ثابت خودڪار& rhs) {
                أرجع cross_قدر_Less(lhs.پهريون, rhs.پهريون);
            });
            std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> merged;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : runs) {
                إذا (merged.خالي_جايون() || cross_قدر_Less(merged.پٺ().ٻيو, لاڳيتو.پهريون)) {
                    merged.ڌڪو_پٺ(لاڳيتو);
                } وإلا إذا (cross_قدر_Less(merged.پٺ().ٻيو, لاڳيتو.ٻيو)) {
                    merged.پٺ().ٻيو = لاڳيتو.ٻيو;
                }
            }
            runs = std::move(merged);
        }
        uint64_t ڳڻپ = 0;
        لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : runs) {
            V3_عدد وسعت{exprp, ويڪر};
            وسعت.opSub(لاڳيتو.ٻيو, لاڳيتو.پهريون);
            إذا (وسعت.most_سيٽ_ٻٽو_انگ_P1() > 32) أرجع سچ;  // More than 2**32 values
            ڳڻپ += وسعت.منزل_UQuad() + 1;
            إذا (ڳڻپ > (uint64_t{1} << 32)) أرجع سچ;
        }
        أرجع خطأ;
    }

    // The ranges of values the wildcard patterns of a sized wildcard array, or of a 'with'
    // filter's candidates, give, counted up to more than --coverage-max-bins
    ساكن سائيز_t sizedWildcardRuns(ثابت AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp) {
        std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> runs;
        لكل (Ast_ڳنڍ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (!VN_IS(rangep, Const)) استمر;  // A range, or a value known at construction
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, exprp)};
            إذا (resolve_قدر(rangep, exprp, سچ, سچ, range) && !cross_Range_خالي_جايون(range)) {
                crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
            }
        }
        أرجع runs.سائيز();
    }

    // Check the automatic bins declarations of a coverpoint.  Each stays one declaration, which
    // generates as a partition of the coverpoint domain (see autoBinRuns).
    فراغ checkAutomaticBins(AstCoverpoint* coverpointp, ثابت Ast_ڳنڍ_اظهار* exprp) {
        لكل (Ast_ڳنڍ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() != VCoverBinsType::BINS_AUTO) استمر;
            ثابت AstConst* ثابت constp = VN_CAST(cbinp->arraySizep(), Const);
            إذا (!constp) {
                cbinp->v3error("Automatic bins array size must be a constant");
            } وإلا إذا (const_ڳڻپ(constp) < 1) {
                cbinp->v3error("Automatic bins array size must be >= 1, got "
                               << constp->انگ().منزل_Decimal_S());
            } وإلا إذا (const_ڳڻپ(constp) > v3Global.opt.coverageMaxBins()) {
                cbinp->v3error("Automatic bins array size of "
                               << constp->انگ().منزل_Decimal_U() << " exceeds limit of "
                               << v3Global.opt.coverageMaxBins() << '\n'
                               << cbinp->warn_وڌيڪ() << "... Suggest a larger --coverage-max-bins");
            } وإلا إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                cbinp->v3error("Automatic bins are not allowed on a coverpoint of a non-integral "
                               "expression (IEEE 1800-2023 19.5.3).");
            }
        }
    }

    // Extract all option values of a coverpoint or cross in a single pass.
    // atLeastOut: option.at_least (default the covergroup's)
    // autoBinMaxOut: option.auto_bin_max (coverpoint overrides covergroup, default 64)
    فراغ ڪڍو_شي_Options(Ast_ڳنڍ* optionsp, صحيح& at_گهٽ_کان_گهٽ_ٻاهريون, صحيح& auto_Bin_Max_ٻاهريون) {
        at_گهٽ_کان_گهٽ_ٻاهريون = m_cg_At_گهٽ_کان_گهٽ;
        auto_Bin_Max_ٻاهريون = -1;  // -1 = not set at coverpoint level
        لكل (Ast_ڳنڍ* optionp = optionsp; optionp; optionp = optionp->nextp()) {
            AstCoverOption* ثابت optp = VN_AS(optionp, CoverOption);
            // Weights may be non-constant; generateItemWeight() handles them.  A cross keeps
            // its unsupported options, which V3LinkParse warned of.
            إذا (!(optp->optType() == VCoverOptionType::AT_گهٽ_کان_گهٽ
                  || optp->optType() == VCoverOptionType::AUTO_BIN_MAX)) {
                استمر;
            }
            ثابت AstConst* ثابت constp = constOption(optp->valuep(), optp->optType());
            إذا (!constp) استمر;
            إذا (optp->optType() == VCoverOptionType::AT_گهٽ_کان_گهٽ) {
                at_گهٽ_کان_گهٽ_ٻاهريون = const_At_گهٽ_کان_گهٽ(constp);
            } وإلا {
                auto_Bin_Max_ٻاهريون = constp->منزل_SInt();
            }
        }
        // Fall back to covergroup-level auto_bin_max if not set at coverpoint level
        إذا (auto_Bin_Max_ٻاهريون < 0) {
            إذا (m_covergroupp->cgAutoBinMax() >= 0) {
                auto_Bin_Max_ٻاهريون = m_covergroupp->cgAutoBinMax();
            } وإلا {
                auto_Bin_Max_ٻاهريون = 64;  // Default per IEEE 1800-2023 Table 19-1
            }
        }
    }

    // IEEE 1800-2023 19.5.2: an enum coverpoint has one automatic bin per enumeration value
    فراغ createEnumAutoBins(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp,
                            ثابت AstEnumDType* enump) {
        File_سٽ* ثابت fl = coverpointp->fileline();
        لكل (ثابت Ast_Enum_شي* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شي)) {
            AstConst* ثابت lop = new_قدر_Const(fl, VN_AS(itemp->valuep(), Const)->انگ(), exprp);
            AstInsideRange* ثابت rangep = جديد AstInsideRange{fl, lop, lop->clone_وڻ(خطأ)};
            rangep->dtypeFrom(exprp);
            coverpointp->جوڙيو_Binsp(
                جديد AstCoverBin{fl, "auto[" + itemp->name() + "]", rangep, خطأ, خطأ});
        }
    }

    // IEEE 1800-2023 19.5.3/19.11.1: partition first, then apply exclusions.  The partition is one
    // automatic bins declaration, generated as a run like 'bins auto[N]' but numbering its bins.
    فراغ createImplicitAutoBins(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp, صحيح autoBinMax) {
        إذا (coverpointp->user2()) أرجع;  // Declared bins, ignored, leave no bins
        لكل (Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت VCoverBinsType قسم = VN_AS(nodep, CoverBin)->binsType();
            إذا (قسم != VCoverBinsType::BINS_IGNORE && قسم != VCoverBinsType::BINS_ILLEGAL)
                أرجع;
        }
        إذا (ثابت AstEnumDType* ثابت enump
            = VN_CAST(exprp->dtypep()->skip_Ref_منزل_Enump(), EnumDType)) {
            createEnumAutoBins(coverpointp, exprp, enump);
            أرجع;
        }
        ثابت صحيح ويڪر = exprp->ويڪر();
        uint32_t ڳڻپ = ويڪر < 31 ? std::min<uint32_t>(uint32_t{1} << ويڪر, autoBinMax)
                                    : static_cast<uint32_t>(autoBinMax);
        إذا (!ڳڻپ) أرجع;
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            coverpointp->v3error("Coverpoint of a non-integral expression requires explicit bins "
                                 "(IEEE 1800-2023 19.5.3).");
            أرجع;
        }
        إذا (ڳڻپ > v3Global.opt.coverageMaxBins()) {
            coverpointp->v3warn(COVERIGN, "Unsupported: more than "
                                              << v3Global.opt.coverageMaxBins()
                                              << " automatic bins from 'option.auto_bin_max'; "
                                                 "using "
                                              << v3Global.opt.coverageMaxBins() << ".\n"
                                              << coverpointp->warn_وڌيڪ()
                                              << "... Suggest a larger --coverage-max-bins");
            ڳڻپ = v3Global.opt.coverageMaxBins();
        }
        File_سٽ* ثابت fl = coverpointp->fileline();
        coverpointp->جوڙيو_Binsp(جديد AstCoverBin{fl, "auto", جديد AstConst{fl, ڳڻپ},
                                              VCoverBinsType::BINS_AUTO_IMPLICIT});
    }

    // Sanitize generated names to be valid C++ identifiers
    ساكن string sanitizeGeneratedName(string name) {
        std::replace(name.begin(), name.پڇاڙي(), '[', '_');
        std::replace(name.begin(), name.پڇاڙي(), ']', '_');
        أرجع name;
    }

    // Capture an iff guard in a function-local temporary so it is evaluated once per sample()
    AstVarRef* capture_Iff_منزل_عارضي(Ast_ڳنڍ_اظهار* iffp, ثابت string& عارضي_Name) {
        File_سٽ* ثابت fl = iffp->fileline();
        AstVar* ثابت iffVarp
            = جديد AstVar{fl, VVarType::BLOCKTEMP, عارضي_Name, iffp->ڳوليو_ٻٽو_انگ_DType()};
        iffVarp->funcLocal(سچ);
        m_sampleFuncp->جوڙيو_Stmtsp(iffVarp);
        iffp->unlink_Fr_پٺ();
        m_sampleFuncp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{fl, جديد AstVarRef{fl, iffVarp, VAccess::WRITE}, iffp});
        أرجع جديد AstVarRef{fl, iffVarp, VAccess::READ};
    }

    // Add a statement sampling the current coverpoint to sample(), under its iff guard if any
    فراغ جوڙيو_Sample_Stmt(Ast_ڳنڍ* stmtp) {
        إذا (m_cpGuardp) {
            m_cpGuardp->جوڙيو_Thensp(stmtp);
        } وإلا {
            m_sampleFuncp->جوڙيو_Stmtsp(stmtp);
        }
    }

    // Create previous value variable for transition tracking
    AstVar* create_Prev_قدر_Var(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp) {
        // Check if already created
        إذا (AstVar* ثابت prevVarp = VN_CAST(coverpointp->user1p(), Var)) أرجع prevVarp;

        // Create variable to store previous sampled value
        ثابت string varName = "__Vprev_" + coverpointp->name();
        AstVar* prevVarp
            = جديد AstVar{coverpointp->fileline(), VVarType::MEMBER, varName, exprp->dtypep()};
        m_covergroupp->جوڙيو_Membersp(prevVarp);

        UINFO(4, "    Created previous value variable: " << varName);

        // Initialize to zero in constructor
        Ast_ڳنڍ_اظهار* ثابت initExprp
            = جديد AstConst{prevVarp->fileline(), AstConst::Widthed_قدر{}, prevVarp->ويڪر(), 0};
        Ast_ڳنڍ_Stmt* ثابت initStmtp = جديد Ast_سونپيو{
            prevVarp->fileline(), جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
            initExprp};
        m_constructorp->جوڙيو_Stmtsp(initStmtp);

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
        m_covergroupp->جوڙيو_Membersp(stateVarp);

        UINFO(4, "    Created sequence state variable: " << varName);

        // Initialize to 0 (not started) in constructor
        Ast_ڳنڍ_Stmt* ثابت initStmtp = جديد Ast_سونپيو{
            stateVarp->fileline(), جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::WRITE},
            جديد AstConst{stateVarp->fileline(), AstConst::Widthed_قدر{}, 8, 0}};
        m_constructorp->جوڙيو_Stmtsp(initStmtp);

        أرجع stateVarp;
    }

    فراغ generate_Coverpoint_ڪوڊ(AstCoverpoint* coverpointp) {
        UINFO(4, "  Generating code for coverpoint: " << coverpointp->name());

        // Get the coverpoint expression
        Ast_ڳنڍ_اظهار* exprp = coverpointp->exprp();

        // Check automatic bins before processing
        checkAutomaticBins(coverpointp, exprp);

        // Extract all coverpoint options in a single pass
        صحيح at_گهٽ_کان_گهٽ_قدر;
        صحيح autoBinMax;
        ڪڍو_شي_Options(coverpointp->optionsp(), at_گهٽ_کان_گهٽ_قدر, autoBinMax);
        UINFO(6, "    Coverpoint at_least = " << at_گهٽ_کان_گهٽ_قدر << " auto_bin_max = " << autoBinMax);

        // Create implicit automatic bins if no regular bins exist
        createImplicitAutoBins(coverpointp, exprp, autoBinMax);

        // A false iff guard disables sampling of the coverpoint (IEEE 1800-2023 19.5), so
        // neither is its expression evaluated, nor are its bins or transition state updated
        VL_RESTORER(m_cpGuardp);
        إذا (Ast_ڳنڍ_اظهار* ثابت iffp = coverpointp->iffp()) {
            m_cpGuardp = جديد AstIf{iffp->fileline(), iffp->unlink_Fr_پٺ()};
        }

        AstVar* ثابت قدر_Varp = جديد AstVar{
            coverpointp->fileline(), VVarType::BLOCKTEMP,
            "__VcpValue_" + sanitizeGeneratedName(coverpointp->name()), exprp->dtypep()};
        قدر_Varp->funcLocal(سچ);
        m_sampleFuncp->جوڙيو_Stmtsp(قدر_Varp);
        exprp->unlink_Fr_پٺ();
        جوڙيو_Sample_Stmt(جديد Ast_سونپيو{
            coverpointp->fileline(),
            جديد AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::WRITE}, exprp});
        coverpointp->exprp(جديد AstVarRef{coverpointp->fileline(), قدر_Varp, VAccess::READ});
        exprp = coverpointp->exprp();

        // Every coverpoint routes through the VlCoverpoint runtime.  Transition coverpoints are
        // included: their per-value matching is still generated as a state machine in sample()
        // (see generateCoverpoint), but the bin hit is recorded in the runtime bin
        // rather than a bare counter.
        generateCoverpoint(coverpointp, exprp, at_گهٽ_کان_گهٽ_قدر);
        // Sampling follows what generateCoverpoint added unguarded, such as clearing the hit list
        إذا (m_cpGuardp) m_sampleFuncp->جوڙيو_Stmtsp(m_cpGuardp);
    }

    // Build the condition under which a default bin matches: NOT(OR of all normal bins).
    Ast_ڳنڍ_اظهار* buildDefaultCondition(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp,
                                       File_سٽ* fl) {
        Ast_ڳنڍ_اظهار* anyBinMatchp = خالي_اشارو;
        لكل (Ast_ڳنڍ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT
                || cbinp->binsType() == VCoverBinsType::BINS_IGNORE
                || cbinp->binsType() == VCoverBinsType::BINS_ILLEGAL)
                استمر;
            // The values of constructed bins are known at construction; see emitSizedSample
            إذا (isConstructedBins(cbinp)) استمر;
            إذا (isAutoBins(cbinp)) {
                // Automatic bins partition the whole domain, leaving no default value
                إذا (anyBinMatchp) VL_DO_DANGLING(ڌڪو_Deletep(anyBinMatchp), anyBinMatchp);
                أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
            }
            Ast_ڳنڍ_اظهار* ثابت binCondp = buildBinCondition(cbinp, exprp);
            UASSERT_OBJ(binCondp, cbinp,
                        "buildBinCondition returned nullptr for non-ignore/non-illegal bin");
            anyBinMatchp = anyBinMatchp ? جديد AstOr{fl, anyBinMatchp, binCondp} : binCondp;
        }
        أرجع anyBinMatchp ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstNot{fl, anyBinMatchp})
                            : static_cast<Ast_ڳنڍ_اظهار*>(جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}});
    }

    //====================================================================
    // VlCoverpoint conversion

    ساكن منطقي coverpointHasStateExclusions(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->transp() && binp->rangesp()
                && (binp->binsType() == VCoverBinsType::BINS_IGNORE
                    || binp->binsType() == VCoverBinsType::BINS_ILLEGAL)) {
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }

    ساكن منطقي coverpointHasConstructedBins(ثابت AstCoverpoint* coverpointp) {
        لكل (ثابت Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            إذا (isConstructedBins(VN_AS(nodep, CoverBin))) أرجع سچ;
        }
        أرجع خطأ;
    }

    // True if a Normal state bin, or an array-bin element, has no value of the coverpoint's
    // type (IEEE 1800-2023 19.5.7).  Array ranges enumerate in-type values, so cannot vanish.
    ساكن منطقي coverpoint_Has_خالي_جايون_Bins(ثابت AstCoverpoint* coverpointp) {
        Ast_ڳنڍ_اظهار* ثابت exprp = coverpointp->exprp();
        لكل (Ast_ڳنڍ* nodep = coverpointp->binsp(); nodep; nodep = nodep->nextp()) {
            ثابت AstCoverBin* ثابت binp = VN_AS(nodep, CoverBin);
            إذا (!binp->binsType().bin_Is_عام() || binp->transp() || !binp->rangesp()) استمر;
            // A wildcard array has a bin for each value its elements match, so none empty, and
            // the constructor creates no bin without values of a 'with' filter
            إذا ((binp->isArray() && binp->isWildcard()) || binWith(binp)) استمر;
            منطقي خالي_جايون = سچ;
            لكل (Ast_ڳنڍ* valuep = binp->rangesp(); valuep; valuep = valuep->nextp()) {
                إذا (binp->isArray() && VN_IS(valuep, InsideRange)) استمر;
                Cross_قدر_Range range{valuep, resolve_ويڪر(valuep, exprp)};
                ثابت منطقي none = resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)
                                  && cross_Range_خالي_جايون(range);
                إذا (binp->isArray() && none) أرجع سچ;
                خالي_جايون &= none;
            }
            إذا (خالي_جايون && !binp->isArray()) أرجع سچ;
        }
        أرجع خطأ;
    }

    // True if a coverpoint has any transition bin.  Used to decide whether sample() emits the
    // end-of-sample previous-value update that transition matching needs.
    ساكن منطقي coverpointHasTransition(AstCoverpoint* coverpointp) {
        لكل (Ast_ڳنڍ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            إذا (VN_AS(binp, CoverBin)->transp()) أرجع سچ;
        }
        أرجع خطأ;
    }

    // The interned AstBasicDType for one of the covergroup runtime keywords.
    ساكن AstBasicDType* basicDType(File_سٽ* fl, VBasicDTypeKwd kwd) {
        أرجع v3Global.rootp()->typeTablep()->ڳوليو_Basic_DType(fl, kwd);
    }

    // Get (or create) the coverpoint dtype for a hit-list bound, interned so each distinct bound
    // yields one node.
    AstCoverpointDType* coverpointDType(File_سٽ* fl, uint32_t ڪاميابي_Bound) {
        AstCoverpointDType*& typep = m_cpDTypes[ڪاميابي_Bound];
        إذا (!typep) {
            typep = جديد AstCoverpointDType{fl, ڪاميابي_Bound};
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(typep);
        }
        أرجع typep;
    }

    // The key of the covergroup's type in the coverage registry, which get_coverage() queries
    std::string covergroupProtectedName() ثابت {
        أرجع VIdProtect::protect_لفظ_If(m_covergroupName, v3Global.opt.protectIds());
    }

    // The name of the covergroup's type, which keys the coverage registry and database: as
    // $typename names it (IEEE 1800-2023 20.6.1), so apart for covergroups of distinct scopes and
    // specializations, and alike in each Verilator run of a hierarchical design.  As design units
    // of distinct libraries may share a name, one of a library other than the default is prefixed
    // by its library, as '%l' prints it (IEEE 1800-2023 33.4).
    std::string covergroupTypeName() ثابت {
        UASSERT_OBJ(m_unitp, m_covergroupp, "Covergroup declared outside of a design unit");
        ثابت std::string& libname = m_unitp->libname();
        ثابت std::string name = m_covergroupp->dtypeName(سچ);
        أرجع libname == "work" ? name : libname + "." + name;
    }

    // Emit the covergroup's instance handle member and the constructor statement that creates
    // its node in the per-context coverage registry.  Runs before any coverpoint or cross is
    // generated, so their runtimes can be added to the node as they are created.
    فراغ generateInstanceAttach() {
        File_سٽ* ثابت fl = m_covergroupp->fileline();
        // V3LinkParse synthesizes a 'new' for every covergroup; the item generators below already
        // rely on that, and this attach runs even for a covergroup with no coverpoints at all.
        UASSERT_OBJ(m_constructorp, m_covergroupp, "Covergroup missing synthesized constructor");
        m_cgInstVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_inst",
                                  basicDType(fl, VBasicDTypeKwd::COVERGROUP_INSTHANDLE)};
        m_covergroupp->جوڙيو_Membersp(m_cgInstVarp);

        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_ATTACH,
                     {ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()"
                                "->newCovergroupInst("
                                    + quoted(covergroupProtectedName())
                                    + (m_cg_May_ملايو ? ", true" : ", false") + ")")},
                     /*usePtr=*/خطأ)
                ->makeStmt());
        // The node reads option.weight in place, so procedural assignments take effect
        AstCExpr* ثابت وزن_Addrp = جديد AstCExpr{fl, "&"};
        وزن_Addrp->جوڙيو(newOptionSel(fl, optionVar(خطأ), "weight", VAccess::READ));
        m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, m_cgInstVarp, VCMethod::COVERGROUP_LEND_وزن,
                                           {وزن_Addrp, file_سٽ_Debug(fl)}, /*usePtr=*/خطأ)
                                      ->makeStmt());
    }

    // A '__Vcg_inst.p()-><method>()' call on the covergroup's instance node
    AstCMethodHard* instanceCall(File_سٽ* fl, VCMethod طريقو) {
        // '__Vcg_inst.p()' -- a value handle, so '.' not '->'
        AstCMethodHard* ثابت instp
            = جديد AstCMethodHard{fl, memberRef(fl, m_cgInstVarp), VCMethod::COVERGROUP_INST_P};
        instp->usePtr(خطأ);
        instp->dtype_سيٽ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, instp, طريقو};
        callp->usePtr(سچ);
        أرجع callp;
    }

    // Emit 'this->__Vcp_x = this->__Vcg_inst.p()->addCoverpoint<K>();' (or addCross), which
    // creates the item runtime in the instance node and borrows a pointer to it.
    Ast_سونپيو* make_شي_Create(File_سٽ* fl, AstVar* شي_Varp, VCMethod طريقو) {
        AstCMethodHard* ثابت createp = instanceCall(fl, طريقو);
        createp->dtypep(شي_Varp->dtypep());
        أرجع جديد Ast_سونپيو{fl, memberRef(fl, شي_Varp, VAccess::WRITE), createp};
    }

    // Constant bounds of one rangesp() element (an InsideRange or a single Const).  Each bound is
    // the raw AST node -- an AstConst or an AstUnbounded ('$') -- with the const/unbounded view
    // derived on demand, so there is one source of truth per bound.  After a successful
    // constRangeBounds() neither node is null; a single Const has both bounds aliasing one node.
    بنية RangeBounds final {
        Ast_ڳنڍ* loNodep = خالي_اشارو;  // low bound: AstConst or AstUnbounded
        Ast_ڳنڍ* hiNodep = خالي_اشارو;  // high bound: AstConst or AstUnbounded
        منطقي loUnbounded() ثابت { أرجع VN_IS(loNodep, Unbounded); }
        منطقي hiUnbounded() ثابت { أرجع VN_IS(hiNodep, Unbounded); }
        AstConst* loConstp() ثابت { أرجع VN_CAST(loNodep, Const); }
        AstConst* hiConstp() ثابت { أرجع VN_CAST(hiNodep, Const); }
    };

    // Decode one rangesp() element into its constant bounds.  Returns false if rp is neither an
    // InsideRange nor a single Const, or if a present bound is non-constant or 4-state.  '$'
    // bounds are left as AstUnbounded (not resolved) -- the caller applies its own policy.
    // Centralizes the InsideRange/Const/Unbounded decode shared by the hit-list-bound paths.
    ساكن منطقي constRangeBounds(Ast_ڳنڍ* rp, RangeBounds& rb) {
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
        إذا ((lc && lc->انگ().isFourState()) || (hc && hc->انگ().isFourState())) أرجع خطأ;
        أرجع سچ;
    }

    // Collect the covered value intervals of a single (non-array) Normal bin.  Returns false
    // if any range isn't a constant/open InsideRange or single Const (e.g. wildcard, non-const).
    ساكن منطقي ڪڍو_Range_Intervals(AstCoverBin* cbinp, uint64_t maxVal,
                                      std::ويڪٽر<std::pair<uint64_t, uint64_t>>& ٻاهريون) {
        إذا (!cbinp->rangesp()) أرجع خطأ;
        لكل (Ast_ڳنڍ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            RangeBounds rb;
            إذا (!constRangeBounds(rp, rb)) أرجع خطأ;
            إذا ((rb.loConstp() && rb.loConstp()->ويڪر() > 64)
                || (rb.hiConstp() && rb.hiConstp()->ويڪر() > 64)) {
                أرجع خطأ;  // Use the safe slot-count bound for wide values.
            }
            ثابت uint64_t lo = rb.loUnbounded() ? 0 : rb.loConstp()->منزل_UQuad();
            ثابت uint64_t hi = rb.hiUnbounded() ? maxVal : rb.hiConstp()->منزل_UQuad();
            إذا (lo > hi) أرجع خطأ;
            ٻاهريون.emplace_پٺ(lo, hi);
        }
        أرجع سچ;
    }

    // Append one Normal bin's cross-slot interval-sets to `bins` and bump `slotCount` by an upper
    // bound of the bin's slots holding one value.  Returns false if any part isn't statically
    // enumerable (the caller then falls back to the always-safe slot count).  A non-array bin
    // is one slot covering the union of its intervals; the bins of an array element or of an
    // automatic bins declaration hold disjoint values, so the element or declaration counts once.
    منطقي appendBinCrossSlots(AstCoverBin* cbinp, uint64_t maxVal, Ast_ڳنڍ_اظهار* exprp,
                             std::ويڪٽر<std::ويڪٽر<std::pair<uint64_t, uint64_t>>>& bins,
                             صحيح& جڳهه_ڳڻپ) {
        إذا (isAutoBins(cbinp)) {
            ++جڳهه_ڳڻپ;
            bins.ڌڪو_پٺ({{0, maxVal}});
            أرجع exprp->ويڪر() <= 64;
        }
        إذا (binWith(cbinp)) {
            // A value is in one bin of a filter's, but of a sized array, in one for each range
            // list element holding it; the coverpoint's name is one element
            صحيح elements = 0;
            لكل (ثابت Ast_ڳنڍ* rp = binRangesp(cbinp); rp && isSizedArray(cbinp);
                 rp = rp->nextp()) {
                ++elements;
            }
            جڳهه_ڳڻپ += std::max(1, elements);
            أرجع خطأ;
        }
        إذا (cbinp->isArray() && cbinp->isWildcard() && !cbinp->arraySizep()) {
            // A value is in at most one bin of a wildcard array: one slot covering its values.
            // Signed values are sign-extended, not unsigned intervals (see computeHitListBound)
            ++جڳهه_ڳڻپ;
            إذا (exprp->isSigned() || exprp->ويڪر() > 64) أرجع خطأ;
            ثابت BinRuns runs = wildcardBinRuns(cbinp, exprp, خطأ);
            إذا (runs.unsupported) أرجع خطأ;
            std::ويڪٽر<std::pair<uint64_t, uint64_t>> ivs;
            لكل (ثابت Bin_لاڳيتو& لاڳيتو : runs.runs) {
                ivs.emplace_پٺ(لاڳيتو.m_lo.منزل_UQuad(), لاڳيتو.m_hi.منزل_UQuad());
            }
            bins.ڌڪو_پٺ(std::move(ivs));
            أرجع سچ;
        }
        إذا (cbinp->isArray()) أرجع appendArrayBinCrossSlots(cbinp, exprp, bins, جڳهه_ڳڻپ);
        // Non-array bin: one slot covering the union of its intervals.
        ++جڳهه_ڳڻپ;
        std::ويڪٽر<std::pair<uint64_t, uint64_t>> ivs;
        إذا (cbinp->isWildcard() || !ڪڍو_Range_Intervals(cbinp, maxVal, ivs)) أرجع خطأ;
        bins.ڌڪو_پٺ(std::move(ivs));
        أرجع سچ;
    }

    // Append the cross slots of an array Normal bin: each element is a slot covering its
    // values, as at most one of its single-value bins holds a value.  An element holds the
    // values arrayBinRuns() gives it: those of the coverpoint type (IEEE 1800-2023 19.5.7).
    // Elements of a signed coverpoint, non-constant elements, and elements with values beyond
    // 64 bits can't be enumerated, so they count one slot but lose exactness.  Returns false if
    // any element wasn't enumerable to exact values.
    منطقي appendArrayBinCrossSlots(AstCoverBin* cbinp, Ast_ڳنڍ_اظهار* exprp,
                                  std::ويڪٽر<std::ويڪٽر<std::pair<uint64_t, uint64_t>>>& bins,
                                  صحيح& جڳهه_ڳڻپ) {
        // Signed values resolve sign-extended, not as unsigned intervals, and wildcard patterns
        // are not intervals
        منطقي exact = !exprp->isSigned() && !cbinp->isWildcard();
        لكل (Ast_ڳنڍ* rp = cbinp->rangesp(); rp; rp = rp->nextp()) {
            ++جڳهه_ڳڻپ;
            RangeBounds rb;
            Cross_قدر_Range range{rp, resolve_ويڪر(rp, exprp)};
            إذا (!exact || !constRangeBounds(rp, rb)
                || !resolve_قدر(rp, exprp, سچ, خطأ, range)) {
                exact = خطأ;
                استمر;
            }
            إذا (cross_Range_خالي_جايون(range)) استمر;  // Its bin, if any, holds no value
            إذا (range.hi.most_سيٽ_ٻٽو_انگ_P1() > 64) {
                exact = خطأ;
                استمر;
            }
            bins.ڌڪو_پٺ({{range.lo.منزل_UQuad(), range.hi.منزل_UQuad()}});
        }
        أرجع exact;
    }

    // Compute the hit-list bound for a coverpoint: the maximum number of Normal
    // bins one sample value can match.  Non-cross-fed coverpoints don't feed a cross, so
    // their hit list is unused -> 1.  Otherwise compute the exact max bin overlap; fall back
    // to the (always-safe) Normal-slot count when any bin isn't statically analyzable.
    صحيح compute_ڪاميابي_List_Bound(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp, منطقي crossFed) {
        إذا (!crossFed) أرجع 1;
        ثابت صحيح ويڪر = exprp->ويڪر();
        ثابت uint64_t maxVal = (ويڪر >= 64) ? UINT64_MAX : ((1ULL << ويڪر) - 1);
        // One entry per Normal bin (cross slot): its covered intervals.
        std::ويڪٽر<std::ويڪٽر<std::pair<uint64_t, uint64_t>>> bins;
        صحيح جڳهه_ڳڻپ = 0;  // == runtime m_normal; the safe fallback bound
        // Unsigned intervals cannot establish overlap between differently sized signed values.
        منطقي exact = !exprp->isSigned();
        لكل (Ast_ڳنڍ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            إذا (!cbinp->binsType().bin_Is_عام())
                استمر;  // ignore/illegal/default: not hit-listed
            إذا (!appendBinCrossSlots(cbinp, maxVal, exprp, bins, جڳهه_ڳڻپ)) exact = خطأ;
        }
        إذا (!exact) أرجع std::max(1, جڳهه_ڳڻپ);
        إذا (bins.خالي_جايون()) أرجع 1;
        // Max overlap occurs at some interval start; count covering bins at each lo.
        std::ويڪٽر<uint64_t> pts;
        لكل (ثابت خودڪار& b : bins)
            لكل (ثابت خودڪار& iv : b) pts.ڌڪو_پٺ(iv.پهريون);
        std::ترتيب_ڏيو(pts.begin(), pts.پڇاڙي());
        pts.erase(std::unique(pts.begin(), pts.پڇاڙي()), pts.پڇاڙي());
        صحيح maxOverlap = 1;
        لكل (ثابت uint64_t p : pts) {
            صحيح cnt = 0;
            لكل (ثابت خودڪار& b : bins) {
                لكل (ثابت خودڪار& iv : b) {
                    إذا (iv.پهريون <= p && p <= iv.ٻيو) {
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
    AstVarRef* memberRef(File_سٽ* fl, AstVar* varp, VAccess access = VAccess::READ) {
        AstVarRef* ثابت refp = جديد AstVarRef{fl, varp, access};
        refp->self_اشارو(VSelf_اشارو_متن{VSelf_اشارو_متن::This{}});
        أرجع refp;
    }

    // A 'this->m_member-><method>(args...)' call on one member.  usePtr is false only for
    // __Vcg_inst, which is a value handle; the item members are borrowed pointers into the
    // instance node.  Numeric arguments are AstConst; the rest are C++ text that has no AST
    // form (see ctext).
    AstCMethodHard* شي_Call(File_سٽ* fl, AstVar* varp, VCMethod طريقو,
                             ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*>& دليلون = {}, منطقي usePtr = سچ) {
        AstCMethodHard* ثابت callp = جديد AstCMethodHard{fl, memberRef(fl, varp), طريقو};
        لكل (Ast_ڳنڍ_اظهار* ثابت argp : دليلون) callp->جوڙيو_Pinsp(argp);
        callp->usePtr(usePtr);
        callp->dtype_سيٽ_Void();
        أرجع callp;
    }

    // An unsigned integer argument.
    ساكن AstConst* cnum(File_سٽ* fl, uint32_t قدر) { أرجع جديد AstConst{fl, قدر}; }

    // A literal C++ argument with no AST equivalent: a 'const char*' string literal (an SV
    // string AstConst emits '"..."s', a std::string temporary the runtime cannot borrow), a
    // VlCovBinKind enum token, a constant selection-word initializer list, a VlFileLineDebug, or
    // a '__V' temporary declared by the enclosing AstCStmt.
    ساكن AstCExpr* ctext(File_سٽ* fl, ثابت std::string& متن) {
        أرجع جديد AstCExpr{fl, متن};
    }

    // A C++ string literal.  Escapes control characters as the emitter does elsewhere -- bin
    // names and filenames reach the generated code verbatim when --protect-ids is off, and an
    // SV escaped identifier may hold a quote or backslash.
    ساكن std::string quoted(ثابت std::string& متن) {
        أرجع "\"" + V3_ٻاهريون_Formatter::quoteNameControls(متن) + "\"";
    }

    // A 'VlFileLineDebug' argument: where the runtime reports an error about fl's construct
    ساكن AstCExpr* file_سٽ_Debug(File_سٽ* fl) {
        ثابت std::string filename
            = VIdProtect::protectIf(fl->filename(), v3Global.opt.protectIds());
        أرجع ctext(fl, "VlFileLineDebug{" + quoted(filename) + ", "
                             + std::منزل_string(fl->lineno()) + "}");
    }

    // Check that an element of an array bin (bins b[] = {values/ranges}) is a two-state
    // constant value or range; false if not, after reporting it if 'report'.
    ساكن منطقي checkArrayBinElement(AstCoverBin* arrayBinp, Ast_ڳنڍ* rangep, منطقي report = سچ) {
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
            إذا ((minp && minp->انگ().isFourState()) || (maxp && maxp->انگ().isFourState())) {
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
        أرجع سچ;
    }

    // Individual equality targets of an array bin (bins b[] = {values/ranges}) of a real
    // coverpoint, in order; integral coverpoints generate array bins as runs (see arrayBinRuns).
    // An open-ended bound ('$', AstUnbounded) resolves to the coverpoint domain: '[lo:$]'
    // covers [lo:maxVal] and '[$:hi]' covers [0:hi].  One target is produced per value; ranges
    // whose resolved size would exceed --coverage-max-real-bins (e.g. an open '[lo:$]') are
    // unsupported -- emits COVERIGN, sets unsupportedOut, yields nothing.
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> ڪڍو_Array_قدرون(AstCoverBin* arrayBinp, Ast_ڳنڍ_اظهار* exprp,
                                                 منطقي& unsupported_ٻاهريون) {
        unsupported_ٻاهريون = خطأ;
        ثابت صحيح ويڪر = exprp->ويڪر();
        ثابت uint64_t maxVal = (ويڪر >= 64) ? UINT64_MAX : ((1ULL << ويڪر) - 1);
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> قدرون;
        لكل (Ast_ڳنڍ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع قدرون;
            إذا (AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange)) {
                ثابت منطقي loUnb = VN_IS(irp->lhsp(), Unbounded);
                ثابت منطقي hiUnb = VN_IS(irp->rhsp(), Unbounded);
                ثابت uint64_t lo = loUnb ? 0 : VN_AS(irp->lhsp(), Const)->منزل_UQuad();
                ثابت uint64_t hi = hiUnb ? maxVal : VN_AS(irp->rhsp(), Const)->منزل_UQuad();
                إذا (hi < lo) استمر;  // empty range contributes no bins
                // Guard against a '$'-bounded or otherwise huge range exploding the bin count.
                ثابت uint64_t وسعت = hi - lo;  // == valueCount - 1 (no overflow: hi >= lo)
                إذا (وسعت >= v3Global.opt.coverageMaxRealBins()
                    || قدرون.سائيز() + وسعت + 1 > v3Global.opt.coverageMaxRealBins()) {
                    arrayBinp->v3warn(COVERIGN,
                                      "Unsupported: array 'bins' of a real coverpoint "
                                      "covering more than "
                                          << v3Global.opt.coverageMaxRealBins() << " values; bin "
                                          << arrayBinp->prettyNameQ() << " ignored.\n"
                                          << arrayBinp->warn_وڌيڪ()
                                          << "... Suggest a larger --coverage-max-real-bins");
                    unsupported_ٻاهريون = سچ;
                    لكل (Ast_ڳنڍ_اظهار* ثابت vp : قدرون) VL_DO_DANGLING(ڌڪو_Deletep(vp), vp);
                    قدرون.clear();
                    أرجع قدرون;
                }
                لكل (uint64_t v = lo; v <= hi; ++v)
                    قدرون.ڌڪو_پٺ(جديد AstConst{irp->fileline(), AstConst::Widthed_قدر{}, ويڪر,
                                                  static_cast<uint32_t>(v)});
            } وإلا {
                قدرون.ڌڪو_پٺ(VN_AS(rangep->clone_وڻ(خطأ), ڳنڍ_اظهار));
            }
        }
        أرجع قدرون;
    }

    ساكن صحيح لاڳيتو_ويڪر(ثابت Ast_ڳنڍ_اظهار* exprp) { أرجع exprp->ويڪر() + 1; }

    // Automatic bins partition the coverpoint domain in value order (IEEE 1800-2023 19.5.3): N
    // bins, capped at the number of values, each hold 2^width / N values, and the last bin also
    // holds the remainder.  False for an invalid declaration, already reported.
    منطقي autoBinRuns(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp, BinRuns& ٻاهريون) {
        ثابت uint32_t requested = autoBinsRequested(binp);
        إذا (!requested || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) أرجع خطأ;
        ثابت صحيح ويڪر = exprp->ويڪر();
        ثابت صحيح arithmetic_ويڪر = لاڳيتو_ويڪر(exprp);
        ثابت uint32_t ڳڻپ
            = ويڪر < 32
                  ? static_cast<uint32_t>(std::min<uint64_t>(uint64_t{1} << ويڪر, requested))
                  : requested;
        Bin_لاڳيتو لاڳيتو{binp, arithmetic_ويڪر, ڳڻپ};
        V3_عدد ڪل{binp, arithmetic_ويڪر};
        ڪل.سيٽ_ٻٽو_انگ(ويڪر, 1);
        لاڳيتو.m_stride.opDiv(ڪل, V3_عدد{binp, arithmetic_ويڪر, ڳڻپ});
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(binp, ويڪر, exprp->isSigned(), arithmetic_ويڪر);
        لاڳيتو.m_lo = domain.lo;
        لاڳيتو.m_hi = domain.hi;
        ٻاهريون.runs.ڌڪو_پٺ(std::move(لاڳيتو));
        ٻاهريون.ڳڻپ = ڳڻپ;
        أرجع سچ;
    }

    // The elements of an array bin (bins b[] = {values/ranges}), in order, as runs of
    // single-value bins.  A range holds the values of the coverpoint type it contains (IEEE
    // 1800-2023 19.5.7), while a singleton names a bin even without such a value.  Errors on a
    // non-constant element.  More than --coverage-max-bins bins (e.g. an open '[lo:$]' range over
    // a wide coverpoint) are unsupported -- emits COVERIGN, and sets unsupported.
    BinRuns arrayBinRuns(AstCoverBin* arrayBinp, Ast_ڳنڍ_اظهار* exprp) {
        BinRuns ٻاهريون;
        ثابت صحيح ويڪر = لاڳيتو_ويڪر(exprp);
        لكل (Ast_ڳنڍ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep)) أرجع ٻاهريون;
            ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, exprp)};
            منطقي خالي_جايون = سچ;
            إذا (!resolve_قدر(rangep, exprp, سچ, خطأ, range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا {
                خالي_جايون = cross_Range_خالي_جايون(range);
            }
            إذا (خالي_جايون && irp) استمر;  // A range without values contributes no bins
            Bin_لاڳيتو لاڳيتو{rangep, ويڪر, 1};
            لاڳيتو.m_خالي_جايون = خالي_جايون;
            uint64_t ڳڻپ = 1;
            إذا (!خالي_جايون) {
                لاڳيتو.m_lo.op_سونپيو(range.lo);
                لاڳيتو.m_hi.op_سونپيو(range.hi);
                V3_عدد وسعت{rangep, ويڪر};
                وسعت.opSub(لاڳيتو.m_hi, لاڳيتو.m_lo);
                // Wider spans exceed any limit
                ڳڻپ = وسعت.most_سيٽ_ٻٽو_انگ_P1() > 32 ? UINT64_MAX : وسعت.منزل_UQuad() + 1;
            }
            إذا (ڳڻپ > v3Global.opt.coverageMaxBins() - ٻاهريون.ڳڻپ) {
                arrayBinp->v3warn(COVERIGN, "Unsupported: array 'bins' covering more than "
                                                << v3Global.opt.coverageMaxBins()
                                                << " values (e.g. an open '[lo:$]' range over "
                                                   "a wide coverpoint); bin "
                                                << arrayBinp->prettyNameQ() << " ignored\n"
                                                << arrayBinp->warn_وڌيڪ()
                                                << "... Suggest a larger --coverage-max-bins");
                ٻاهريون.runs.clear();
                ٻاهريون.ڳڻپ = 0;
                ٻاهريون.unsupported = سچ;
                أرجع ٻاهريون;
            }
            لاڳيتو.m_ڳڻپ = static_cast<uint32_t>(ڳڻپ);
            ٻاهريون.ڳڻپ += لاڳيتو.m_ڳڻپ;
            ٻاهريون.runs.ڌڪو_پٺ(std::move(لاڳيتو));
        }
        أرجع ٻاهريون;
    }

    // The bins of a wildcard array (wildcard bins b[] = {...}): one for each coverpoint value
    // an element matches (IEEE 1800-2023 19.5.4, 19.5.7), in value order, and named by the value
    // (19.5.1), as runs of single-value bins.  Errors on a non-constant element.  More than
    // --coverage-max-bins values are unsupported -- emits COVERIGN, and sets unsupported, and for
    // an ignore or illegal array, single.  'report' false omits these diagnostics.
    ساكن BinRuns wildcardBinRuns(AstCoverBin* arrayBinp, Ast_ڳنڍ_اظهار* exprp, منطقي report) {
        BinRuns ٻاهريون;
        ثابت صحيح ويڪر = لاڳيتو_ويڪر(exprp);
        ثابت V3_عدد one{arrayBinp, ويڪر, 1};
        // Disjoint runs of the values, in value order, each value once
        std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> spans;
        uint64_t ڳڻپ = 0;  // Values of 'spans'
        لكل (Ast_ڳنڍ* rangep = arrayBinp->rangesp(); rangep; rangep = rangep->nextp()) {
            rangep = V3Const::constifyEdit(rangep);
            إذا (!checkArrayBinElement(arrayBinp, rangep, report)) {
                ٻاهريون.unsupported = سچ;
                أرجع ٻاهريون;
            }
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, سچ, range)) {
                إذا (report) {
                    rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a "
                                                  "coverage bin of an integral coverpoint.");
                }
                استمر;
            }
            إذا (cross_Range_خالي_جايون(range)) استمر;
            std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> مليو;
            crossRangeRuns(range, v3Global.opt.coverageMaxBins(), مليو);
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : مليو) {
                // Coverpoint values, sign-extended in both widths
                spans.emplace_پٺ(V3_عدد{rangep, ويڪر, لاڳيتو.پهريون},
                                   V3_عدد{rangep, ويڪر, لاڳيتو.ٻيو});
            }
            std::ترتيب_ڏيو(spans.begin(), spans.پڇاڙي(), [](ثابت خودڪار& lhs, ثابت خودڪار& rhs) {
                أرجع cross_قدر_Less(lhs.پهريون, rhs.پهريون);
            });
            std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> merged;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& وسعت : spans) {
                // Adjacent or overlapping values join a run; 'first - 1' cannot overflow
                V3_عدد before{rangep, ويڪر};
                before.opSub(وسعت.پهريون, one);
                إذا (merged.خالي_جايون() || cross_قدر_Less(merged.پٺ().ٻيو, before)) {
                    merged.ڌڪو_پٺ(وسعت);
                } وإلا إذا (cross_قدر_Less(merged.پٺ().ٻيو, وسعت.ٻيو)) {
                    merged.پٺ().ٻيو = وسعت.ٻيو;
                }
            }
            ڳڻپ = 0;
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& وسعت : merged) {
                V3_عدد سائيز{rangep, ويڪر};
                سائيز.opSub(وسعت.ٻيو, وسعت.پهريون);
                // Beyond 2^32 values exceed any limit
                ڳڻپ += سائيز.most_سيٽ_ٻٽو_انگ_P1() > 32 ? uint64_t{1} << 33 : سائيز.منزل_UQuad() + 1;
                إذا (ڳڻپ > v3Global.opt.coverageMaxBins()) اكسر;
            }
            spans = std::move(merged);
            إذا (ڳڻپ > v3Global.opt.coverageMaxBins()) {
                // An ignore or illegal array still excludes or checks its values, as one bin
                ٻاهريون.single = !arrayBinp->binsType().bin_Is_عام();
                إذا (report) {
                    arrayBinp->v3warn(
                        COVERIGN, "Unsupported: wildcard array '"
                                      << arrayBinp->binsType().verilogKwd()
                                      << "' of more than --coverage-max-bins of "
                                      << v3Global.opt.coverageMaxBins() << " values; bin "
                                      << arrayBinp->prettyNameQ()
                                      << (ٻاهريون.single ? " treated as one bin" : " ignored") << "\n"
                                      << arrayBinp->warn_وڌيڪ()
                                      << "... Suggest a larger --coverage-max-bins");
                }
                ٻاهريون.unsupported = سچ;
                أرجع ٻاهريون;
            }
        }
        لكل (ثابت std::pair<V3_عدد, V3_عدد>& وسعت : spans) {
            ٻاهريون.runs.emplace_پٺ(arrayBinp, ويڪر, 0);
            Bin_لاڳيتو& لاڳيتو = ٻاهريون.runs.پٺ();
            لاڳيتو.m_lo = وسعت.پهريون;
            لاڳيتو.m_hi = وسعت.ٻيو;
            // A bin for each value, named by it in the coverpoint's type
            V3_عدد قدر = وسعت.پهريون;
            طالما (سچ) {
                ثابت V3_عدد typed{arrayBinp, exprp->ويڪر(), قدر};
                ٻاهريون.قدرون.ڌڪو_پٺ(exprp->isSigned() ? typed.منزل_Decimal_S() : typed.منزل_Decimal_U());
                ++لاڳيتو.m_ڳڻپ;
                إذا (قدر.isCaseEq(وسعت.ٻيو)) اكسر;
                V3_عدد ايندڙ{arrayBinp, ويڪر};
                قدر = ايندڙ.op_جوڙيو(قدر, one);
            }
        }
        ٻاهريون.ڳڻپ = static_cast<uint32_t>(ڳڻپ);
        أرجع ٻاهريون;
    }

    // The runs of an automatic bins declaration, or of an array bin of an integral coverpoint.
    // False for other bins, which do not generate as runs, including a wildcard array then one
    // bin (see BinRuns::single).
    منطقي binRunsFor(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp, BinRuns& ٻاهريون) {
        إذا (isAutoBins(binp)) {
            إذا (!autoBinRuns(binp, exprp, ٻاهريون)) ٻاهريون.unsupported = سچ;
            أرجع سچ;
        }
        إذا (binp->isArray() && binp->isWildcard()) {
            إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
                Ast_ڳنڍ_اظهار* ثابت falsep = wildcardTypeError(binp, exprp);
                VL_DO_DANGLING(ڌڪو_Deletep(falsep), falsep);
                ٻاهريون.unsupported = سچ;
            } وإلا {
                ٻاهريون = wildcardBinRuns(binp, exprp, سچ);
                إذا (ٻاهريون.single) {
                    binp->isArray(خطأ);  // Generates as one bin
                    أرجع خطأ;
                }
            }
            أرجع سچ;
        }
        إذا (!binp->isArray() || binp->transp() || binp->isWildcard()
            || !exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع خطأ;
        }
        ٻاهريون = arrayBinRuns(binp, exprp);
        أرجع سچ;
    }

    // Emit a 'this->m_cp->addSingleNamer/addArrayNamer(...)' statement for one bin whose first
    // runtime bin index is 'declared'; or with 'valueNames', those naming each bin of an array
    Ast_ڳنڍ_Stmt* makeNamer(AstVar* cpVarp, AstCoverBin* binp, int64_t ڳڻپ, uint32_t declared,
                           ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*>& قدرون = {},
                           ثابت std::ويڪٽر<std::string>& قدر_نالا = {}) {
        File_سٽ* ثابت fl = binp->fileline();
        CoverpointBins& bins = m_cpBins.at(cpVarp);
        ثابت uint32_t عام_ڳڻپ
            = binp->binsType().bin_Is_عام() ? static_cast<uint32_t>(ڳڻپ < 0 ? 1 : ڳڻپ) : 0;
        ثابت Bin_وسعت وسعت{bins.ڪل, عام_ڳڻپ, declared};
        إذا (binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT) {
            bins.implicitAuto = وسعت;  // Selected by bin, see implicitAutoBinSpan
        } وإلا {
            bins.spans.emplace(binp->name(), وسعت);
        }
        bins.ڪل += عام_ڳڻپ;
        لكل (uint32_t i = 0; bins.crossed && i < عام_ڳڻپ; ++i) {
            bins.قدرون.ڌڪو_پٺ({binp, قدرون.خالي_جايون() ? خالي_اشارو : قدرون[i]});
        }
        // Under --protect-ids the filename and bin name flow into the coverage database
        // verbatim, so obfuscate them exactly as line/toggle coverage points are (whole-
        // unit filename, per-word bin name).  A no-op when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت منطقي single = ڳڻپ < 0;
        إذا (!قدر_نالا.خالي_جايون()) {
            // A bin of a wildcard array is named by its value (IEEE 1800-2023 19.5.1)
            Ast_ڳنڍ_Stmt* stmtsp = خالي_اشارو;
            لكل (ثابت std::string& قدر : قدر_نالا) {
                ثابت std::string name
                    = VIdProtect::protect_لفظ_If(binp->name(), prot) + "[" + قدر + "]";
                stmtsp = Ast_ڳنڍ::جوڙيو_ايندڙ(
                    stmtsp,
                    شي_Call(fl, cpVarp, VCMethod::COVERGROUP_جوڙيو_SINGLE_NAMER,
                             {ctext(fl, binp->binsType().bin_سيٽ_Enum()), ctext(fl, quoted(name)),
                              ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                              cnum(fl, static_cast<uint32_t>(fl->lineno())),
                              cnum(fl, static_cast<uint32_t>(fl->پهريون_ڪالم()))})
                        ->makeStmt());
            }
            أرجع stmtsp;
        }
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> دليلون{ctext(fl, binp->binsType().bin_سيٽ_Enum())};
        إذا (!single) دليلون.ڌڪو_پٺ(cnum(fl, static_cast<uint32_t>(ڳڻپ)));  // value array bin
        دليلون.ڌڪو_پٺ(ctext(fl, quoted(VIdProtect::protect_لفظ_If(binp->name(), prot))));
        دليلون.ڌڪو_پٺ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دليلون.ڌڪو_پٺ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دليلون.ڌڪو_پٺ(cnum(fl, static_cast<uint32_t>(fl->پهريون_ڪالم())));
        أرجع شي_Call(fl, cpVarp,
                        single ? VCMethod::COVERGROUP_جوڙيو_SINGLE_NAMER
                        : binp->binsType() == VCoverBinsType::BINS_AUTO_IMPLICIT
                            ? VCMethod::COVERGROUP_جوڙيو_NUMBERED_NAMER
                            : VCMethod::COVERGROUP_جوڙيو_ARRAY_NAMER,
                        دليلون)
            ->makeStmt();
    }

    // Emit 'if (iff && cond) m_cp.incrementBin(idx);' (or recordHit, + illegal action) in sample()
    // Where a bin's hit is recorded in the runtime VlCoverpoint member.
    بنية Conv_Bin_هدف final {
        AstVar* cpVarp;  // the __Vcp_<coverpoint> member
        uint32_t idx;  // bin index within that coverpoint
        منطقي is_عام;  // Normal -> incrementBin (count + cross hit list); else recordHit (count)
    };

    // Emit 'this->m_cp.incrementBin(idx);' (Normal) or '.recordHit(idx);'
    // (ignore/illegal/default).
    Ast_ڳنڍ_Stmt* make_Runtime_Bin_ڪاميابي(File_سٽ* fl, AstVar* cpVarp, Ast_ڳنڍ_اظهار* idxp,
                                   منطقي is_عام) {
        أرجع شي_Call(fl, cpVarp,
                        is_عام ? VCMethod::COVERGROUP_INCREMENT_BIN
                                 : VCMethod::COVERGROUP_رڪارڊ_ڪاميابي,
                        {idxp})
            ->makeStmt();
    }
    Ast_ڳنڍ_Stmt* make_Runtime_Bin_ڪاميابي(File_سٽ* fl, ثابت Conv_Bin_هدف& tgt) {
        أرجع make_Runtime_Bin_ڪاميابي(fl, tgt.cpVarp, cnum(fl, static_cast<uint32_t>(tgt.idx)),
                                 tgt.is_عام);
    }

    // The condition under which a bin counts a sample: condp, and the bin's iff, and for a
    // Normal or default state bin, that the value is not excluded
    Ast_ڳنڍ_اظهار* binCondition(AstCoverBin* binp, AstVar* cpVarp, Ast_ڳنڍ_اظهار* condp) {
        File_سٽ* ثابت fl = binp->fileline();
        إذا (binp->iffp()) condp = جديد AstLogAnd{fl, binp->iffp()->clone_وڻ(خطأ), condp};
        ثابت خودڪار excluded = m_excludedVars.ڳوليو(cpVarp);
        إذا (excluded != m_excludedVars.پڇاڙي() && !binp->transp()
            && (binp->binsType().bin_Is_عام()
                || binp->binsType() == VCoverBinsType::BINS_DEFAULT)) {
            condp = جديد AstLogAnd{
                fl, جديد AstNot{fl, جديد AstVarRef{fl, excluded->ٻيو, VAccess::READ}}, condp};
        }
        أرجع condp;
    }

    فراغ emit_Conv_ڪاميابي_If(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                       Ast_ڳنڍ_اظهار* idxp, Ast_ڳنڍ_اظهار* condp) {
        File_سٽ* ثابت fl = binp->fileline();
        Ast_ڳنڍ* actionp = make_Runtime_Bin_ڪاميابي(fl, cpVarp, idxp, binp->binsType().bin_Is_عام());
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڙيو_ايندڙ(makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                          + " hit in coverpoint "
                                                          + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        جوڙيو_Sample_Stmt(جديد AstIf{fl, binCondition(binp, cpVarp, condp), actionp, خالي_اشارو});
    }

    // Emit the sample() code of sized array 'sized': count the value in its bins holding it when
    // enabled, as other bins are, and for default bins note in matchedp whether any holds it
    فراغ emitSizedSample(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                         Ast_ڳنڍ_اظهار* exprp, uint32_t sized, AstVar* matchedp) {
        File_سٽ* ثابت fl = binp->fileline();
        Ast_ڳنڍ_اظهار* enabledp = binCondition(binp, cpVarp, جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}});
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for coverpoint");
        ثابت منطقي illegal = binp->binsType() == VCoverBinsType::BINS_ILLEGAL;
        إذا (illegal) {
            // The illegal action reads the condition too, which is evaluated once, as the guard
            // may have side effects
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpEnabled_" + sanitizeGeneratedName(coverpointp->name()) + "_"
                                 + cvt_منزل_Str(sized),
                             binp->ڳوليو_ٻٽو_انگ_DType()};
            varp->funcLocal(سچ);
            m_sampleFuncp->جوڙيو_Stmtsp(varp);
            جوڙيو_Sample_Stmt(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, enabledp});
            enabledp = جديد AstVarRef{fl, varp, VAccess::READ};
        }
        AstCMethodHard* ثابت callp
            = شي_Call(fl, cpVarp,
                       exprp->isWide() ? VCMethod::COVERGROUP_SIZED_SAMPLE_W
                                       : VCMethod::COVERGROUP_SIZED_SAMPLE,
                       {cnum(fl, sized), exprp->clone_وڻ(خطأ), enabledp});
        callp->dtype_سيٽ_ٻٽو_انگ();
        إذا (illegal) {
            جوڙيو_Sample_Stmt(جديد AstIf{fl, جديد AstLogAnd{fl, callp, enabledp->clone_وڻ(خطأ)},
                                    makeIllegalBinAction(fl, "Illegal bin " + binp->prettyNameQ()
                                                                 + " hit in coverpoint "
                                                                 + coverpointp->prettyNameQ())});
        } وإلا إذا (matchedp && binp->binsType().bin_Is_عام()) {
            جوڙيو_Sample_Stmt(
                جديد Ast_سونپيو{fl, جديد AstVarRef{fl, matchedp, VAccess::WRITE},
                              جديد AstOr{fl, جديد AstVarRef{fl, matchedp, VAccess::READ}, callp}});
        } وإلا {
            جوڙيو_Sample_Stmt(callp->makeStmt());
        }
    }

    // A variable local to the constructor
    AstVar* constructor_عارضي(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_DType* dtypep) {
        AstVar* ثابت varp = جديد AstVar{fl, VVarType::BLOCKTEMP, name, dtypep};
        varp->funcLocal(سچ);
        m_constructorp->جوڙيو_Stmtsp(varp);
        أرجع varp;
    }

    // Truncate or extend, as its signedness sets, a value to a type
    ساكن Ast_ڳنڍ_اظهار* resize_قدر(Ast_ڳنڍ_اظهار* valuep, Ast_ڳنڍ_DType* dtypep) {
        File_سٽ* ثابت fl = valuep->fileline();
        إذا (valuep->ويڪر() > dtypep->ويڪر()) {
            valuep = جديد AstSel{fl, valuep, 0, dtypep->ويڪر()};
        } وإلا إذا (valuep->ويڪر() < dtypep->ويڪر()) {
            valuep = valuep->isSigned()
                         ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstExtendS{fl, valuep, dtypep->ويڪر()})
                         : جديد AstExtend{fl, valuep, dtypep->ويڪر()};
        }
        valuep->dtypep(dtypep);
        أرجع valuep;
    }

    // The runtime arguments of a count, which the constructor evaluates once into a temporary:
    // its value, a count wider than 64 bits saturated over any limit, and whether it is positive
    std::pair<Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*> new_ڳڻپ_دليلون(File_سٽ* fl, ثابت string& name,
                                                       Ast_ڳنڍ_اظهار* exprp) {
        AstVar* ثابت countp = constructor_عارضي(fl, name, exprp->dtypep());
        m_constructorp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{fl, جديد AstVarRef{fl, countp, VAccess::WRITE}, exprp->clone_وڻ(خطأ)});
        ثابت خودڪار ڳڻپ_Ref = [&]() { أرجع جديد AstVarRef{fl, countp, VAccess::READ}; };
        AstConst* ثابت zerop = جديد AstConst{fl, AstConst::DTyped{}, countp->dtypep()};
        Ast_ڳنڍ_اظهار* ثابت positivep
            = countp->isSigned() ? static_cast<Ast_ڳنڍ_اظهار*>(جديد AstGtS{fl, ڳڻپ_Ref(), zerop})
                                 : جديد AstNeq{fl, ڳڻپ_Ref(), zerop};
        Ast_ڳنڍ_اظهار* valuep = resize_قدر(ڳڻپ_Ref(), countp->ڳوليو_UInt64_DType());
        إذا (countp->ويڪر() > VL_QUADSIZE) {
            valuep = جديد AstCond{
                fl,
                جديد AstRedOr{
                    fl, جديد AstSel{fl, ڳڻپ_Ref(), VL_QUADSIZE, countp->ويڪر() - VL_QUADSIZE}},
                جديد AstConst{fl, AstConst::Unsized64{}, std::numeric_limits<uint64_t>::max()},
                valuep};
            valuep->dtype_سيٽ_UInt64();
        }
        أرجع {valuep, positivep};
    }
    // The runtime arguments of a constant count
    ساكن std::pair<Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*> new_ڳڻپ_دليلون(File_سٽ* fl, uint64_t ڳڻپ) {
        أرجع {جديد AstConst{fl, AstConst::Unsized64{}, ڳڻپ},
                جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}};
    }

    // The values of a coverpoint, which its name denotes (IEEE 1800-2023 19.5.1.1), as runs in
    // value order: an enumerated type's values (6.19), else all values of its type.  Bounds
    // are at runWidth(), sign-extended like a CrossValueRange's.
    ساكن std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> coverpoint_قدرون(Ast_ڳنڍ* nodep,
                                                                       Ast_ڳنڍ_اظهار* exprp) {
        ثابت صحيح ويڪر = لاڳيتو_ويڪر(exprp);
        std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> runs;
        ثابت AstEnumDType* ثابت enump = VN_CAST(exprp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enump) {
            ثابت Cross_قدر_Range domain
                = cross_قدر_Domain(nodep, exprp->ويڪر(), exprp->isSigned(), ويڪر);
            runs.emplace_پٺ(domain.lo, domain.hi);
            أرجع runs;
        }
        std::ويڪٽر<V3_عدد> قدرون;
        لكل (ثابت Ast_Enum_شي* itemp = enump->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_شي)) {
            ثابت V3_عدد& انگ = VN_AS(itemp->valuep(), Const)->انگ();
            إذا (انگ.isFourState()) استمر;  // Not a coverpoint value (19.5.7)
            قدرون.emplace_پٺ(nodep, ويڪر);
            إذا (exprp->isSigned()) {
                قدرون.پٺ().opExtendS(انگ, انگ.ويڪر());
            } وإلا {
                قدرون.پٺ().op_سونپيو(انگ);
            }
        }
        std::ترتيب_ڏيو(قدرون.begin(), قدرون.پڇاڙي(), cross_قدر_Less);
        ثابت V3_عدد one{nodep, ويڪر, 1};
        لكل (ثابت V3_عدد& قدر : قدرون) {
            V3_عدد ايندڙ{nodep, ويڪر};
            إذا (!runs.خالي_جايون() && ايندڙ.op_جوڙيو(runs.پٺ().ٻيو, one).isCaseEq(قدر)) {
                runs.پٺ().ٻيو = قدر;
            } وإلا {
                runs.emplace_پٺ(قدر, قدر);
            }
        }
        أرجع runs;
    }

    // Emit the constructor code building the bins 'binp' whose values it computes: the values of
    // each element that are coverpoint values (IEEE 1800-2023 19.5.7), those a 'with' filter
    // keeps (19.5.1.1), then its bins
    فراغ generateConstructedBins(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                                 Ast_ڳنڍ_اظهار* exprp) {
        File_سٽ* ثابت fl = binp->fileline();
        ثابت string اڳياڙي
            = m_sized_نالا.get(sanitizeGeneratedName(coverpointp->name() + "__" + binp->name()));
        // The count, and whether it is positive; one of a filter's scalar bin, or bin per value
        ثابت std::pair<Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*> ڳڻپ
            = binp->arraySizep() ? new_ڳڻپ_دليلون(fl, اڳياڙي + "_count", binp->arraySizep())
                                 : new_ڳڻپ_دليلون(fl, 1);
        AstCoverWith* ثابت withp = binWith(binp);
        إذا (withp && !binRangesp(binp)) {  // The coverpoint's name: all of its values
            لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : coverpoint_قدرون(binp, exprp)) {
                m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cpVarp,
                                                   exprp->isWide()
                                                       ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                       : VCMethod::COVERGROUP_SIZED_RANGE,
                                                   {new_قدر_Const(fl, لاڳيتو.پهريون, exprp),
                                                    new_قدر_Const(fl, لاڳيتو.ٻيو, exprp)})
                                              ->makeStmt());
            }
        }
        uint32_t element = 0;
        لكل (Ast_ڳنڍ* rangep = binRangesp(binp); rangep; rangep = rangep->nextp()) {
            إذا (VN_IS(rangep, Unbounded)) {  // A parameter of '$'
                binp->v3error("Bins value may not be '$', which may only bound a range "
                              "(IEEE 1800-2023 6.20.7)");
                استمر;
            }
            generateSizedElement(cpVarp, binp, rangep, exprp, اڳياڙي + "_" + cvt_منزل_Str(element++));
        }
        إذا (withp) generateWithFilter(binp, withp, cpVarp, exprp, اڳياڙي);
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> دليلون{ctext(fl, binp->binsType().bin_سيٽ_Enum()), ڳڻپ.پهريون,
                                       ڳڻپ.ٻيو};
        // A filter's bins have the limit it began with
        إذا (!withp) دليلون.ڌڪو_پٺ(cnum(fl, v3Global.opt.coverageMaxBins()));
        ثابت منطقي prot = v3Global.opt.protectIds();
        دليلون.ڌڪو_پٺ(ctext(fl, quoted(VIdProtect::protect_لفظ_If(binp->name(), prot))));
        دليلون.ڌڪو_پٺ(ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))));
        دليلون.ڌڪو_پٺ(cnum(fl, static_cast<uint32_t>(fl->lineno())));
        دليلون.ڌڪو_پٺ(cnum(fl, static_cast<uint32_t>(fl->پهريون_ڪالم())));
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cpVarp,
                     withp ? VCMethod::COVERGROUP_WITH_پڄاڻي : VCMethod::COVERGROUP_SIZED_پڄاڻي,
                     دليلون)
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
                            Ast_ڳنڍ_اظهار* exprp, ثابت string& اڳياڙي) {
        File_سٽ* ثابت fl = withp->fileline();
        ثابت منطقي wide = exprp->isWide();
        ثابت string grouping = !binp->isArray()     ? "Single"
                                : binp->arraySizep() ? "Fixed"
                                                     : "Values";
        m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_BEGIN,
                                           {ctext(fl, "VlCovBinGrouping::" + grouping),
                                            cnum(fl, v3Global.opt.coverageMaxBins())})
                                      ->makeStmt());
        // The candidates count in the coverpoint's width, and the filter reads each as 'item',
        // of the coverpoint's type, so that a filter changing 'item' cannot change the loop
        Ast_ڳنڍ_DType* ثابت قدر_DTypep = exprp->dtypep();
        AstVar* ثابت valuep = constructor_عارضي(fl, اڳياڙي + "_value", قدر_DTypep);
        AstVar* ثابت lastp = constructor_عارضي(fl, اڳياڙي + "_last", قدر_DTypep);
        AstVar* ثابت firstp = constructor_عارضي(fl, اڳياڙي + "_first", قدر_DTypep);
        AstVar* ثابت runp = constructor_عارضي(fl, اڳياڙي + "_run", binp->ڳوليو_ٻٽو_انگ_DType());
        AstVar* ثابت morep = constructor_عارضي(fl, اڳياڙي + "_more", binp->ڳوليو_ٻٽو_انگ_DType());
        AstVar* ثابت itemp = withp->itemp()->unlink_Fr_پٺ();
        itemp->name(اڳياڙي + "_item");
        m_constructorp->جوڙيو_Stmtsp(itemp);
        ثابت خودڪار ref = [&](AstVar* varp) { أرجع جديد AstVarRef{fl, varp, VAccess::READ}; };
        ثابت خودڪار سونپيو_2 = [&](AstVar* varp, Ast_ڳنڍ_اظهار* rhsp) -> Ast_ڳنڍ* {
            أرجع جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE}, rhsp};
        };
        ثابت خودڪار flag = [&](AstVar* varp, منطقي قدر) {
            أرجع سونپيو_2(varp, قدر ? جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}}
                                      : جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}});
        };
        ثابت خودڪار bound = [&](VCMethod narrow, VCMethod wide_طريقو, AstVar* varp) -> Ast_ڳنڍ* {
            إذا (wide) {
                أرجع شي_Call(fl, cpVarp, wide_طريقو, {جديد AstVarRef{fl, varp, VAccess::WRITE}})
                    ->makeStmt();
            }
            AstCMethodHard* ثابت callp = شي_Call(fl, cpVarp, narrow);
            callp->dtype_سيٽ_UInt64();
            أرجع سونپيو_2(varp, resize_قدر(callp, قدر_DTypep));
        };
        ثابت خودڪار keep = [&](Ast_ڳنڍ_اظهار* lop, Ast_ڳنڍ_اظهار* hip) {
            AstCMethodHard* ثابت callp = شي_Call(
                fl, cpVarp, wide ? VCMethod::COVERGROUP_WITH_لاڳيتو_W : VCMethod::COVERGROUP_WITH_لاڳيتو,
                {lop, hip});
            callp->dtype_سيٽ_ٻٽو_انگ();
            أرجع سونپيو_2(morep, callp);
        };
        ثابت خودڪار قدم = [&](منطقي مٿي) {
            AstConst* ثابت onep = جديد AstConst{fl, AstConst::Widthed_قدر{}, exprp->ويڪر(), 1};
            Ast_ڳنڍ_اظهار* ثابت stepp
                = مٿي ? static_cast<Ast_ڳنڍ_اظهار*>(جديد Ast_جوڙيو{fl, ref(valuep), onep})
                     : جديد AstSub{fl, ref(valuep), onep};
            stepp->dtypep(قدر_DTypep);
            أرجع stepp;
        };
        AstLoop* ثابت innerp = جديد AstLoop{fl};
        innerp->جوڙيو_Stmtsp(سونپيو_2(itemp, ref(valuep)));
        innerp->جوڙيو_Stmtsp(جديد AstIf{
            fl, withp->filterp()->unlink_Fr_پٺ(),
            جديد AstIf{fl, جديد AstNot{fl, ref(runp)},
                      سونپيو_2(firstp, ref(valuep))->جوڙيو_ايندڙ(flag(runp, سچ))},
            جديد AstIf{fl, ref(runp), keep(ref(firstp), قدم(خطأ))->جوڙيو_ايندڙ(flag(runp, خطأ))}});
        innerp->جوڙيو_Stmtsp(جديد AstLoopTest{
            fl, innerp, جديد AstLogAnd{fl, ref(morep), جديد AstNeq{fl, ref(valuep), ref(lastp)}}});
        innerp->جوڙيو_Stmtsp(سونپيو_2(valuep, قدم(سچ)));
        AstLoop* ثابت outerp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = شي_Call(fl, cpVarp, VCMethod::COVERGROUP_WITH_ايندڙ);
        nextp->dtype_سيٽ_ٻٽو_انگ();
        outerp->جوڙيو_Stmtsp(جديد AstLoopTest{fl, outerp, nextp});
        outerp->جوڙيو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_LO, VCMethod::COVERGROUP_WITH_LO_W, valuep));
        outerp->جوڙيو_Stmtsp(
            bound(VCMethod::COVERGROUP_WITH_HI, VCMethod::COVERGROUP_WITH_HI_W, lastp));
        outerp->جوڙيو_Stmtsp(flag(runp, خطأ));
        outerp->جوڙيو_Stmtsp(innerp);
        outerp->جوڙيو_Stmtsp(جديد AstIf{fl, ref(runp), keep(ref(firstp), ref(lastp))});
        m_constructorp->جوڙيو_Stmtsp(flag(morep, سچ));
        m_constructorp->جوڙيو_Stmtsp(outerp);
    }

    // Emit 'sizedRange(lo, hi)' for the coverpoint values of an element of bins 'binp', whose
    // values the constructor computes: resolved now if constant, else when constructed by
    // clipping to the coverpoint's values.  A wildcard pattern's values give a range for each
    // run of them.  'prefix' names its temporaries.
    فراغ generateSizedElement(AstVar* cpVarp, ثابت AstCoverBin* binp, Ast_ڳنڍ* rangep,
                              Ast_ڳنڍ_اظهار* exprp, ثابت string& اڳياڙي) {
        File_سٽ* ثابت fl = rangep->fileline();
        ثابت VCMethod طريقو = exprp->isWide() ? VCMethod::COVERGROUP_SIZED_RANGE_W
                                                : VCMethod::COVERGROUP_SIZED_RANGE;
        ثابت AstInsideRange* ثابت irp = VN_CAST(rangep, InsideRange);
        Ast_ڳنڍ_اظهار* ثابت lowp = irp ? irp->lhsp() : VN_AS(rangep, ڳنڍ_اظهار);
        Ast_ڳنڍ_اظهار* ثابت highp = irp ? irp->rhsp() : خالي_اشارو;
        ثابت خودڪار unbounded
            = [](ثابت Ast_ڳنڍ_اظهار* boundp) { أرجع !boundp || VN_IS(boundp, Unbounded); };
        ثابت خودڪار constant
            = [&](ثابت Ast_ڳنڍ_اظهار* boundp) { أرجع unbounded(boundp) || VN_IS(boundp, Const); };
        ثابت خودڪار integral = [&](ثابت Ast_ڳنڍ_اظهار* boundp) {
            أرجع unbounded(boundp) || boundp->dtypep()->skipRefp()->isIntegralOrPacked();
        };
        إذا (constant(lowp) && constant(highp)) {
            ثابت خودڪار fourState = [](ثابت Ast_ڳنڍ_اظهار* boundp) {
                ثابت AstConst* ثابت constp = VN_CAST(boundp, Const);
                أرجع constp && constp->انگ().isFourState();
            };
            إذا (irp && (fourState(lowp) || fourState(highp))) {
                rangep->v3error("Four-state (x/z) value in "
                                << (binp->isArray() ? "array bins" : "bin")
                                << " range bound; range bounds must be two-state constants");
                أرجع;
            }
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, exprp)};
            إذا (!resolve_قدر(rangep, exprp, سچ, binp->isWildcard(), range)) {
                rangep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a coverage bin "
                                              "of an integral coverpoint.");
            } وإلا إذا (!cross_Range_خالي_جايون(range)) {
                std::ويڪٽر<std::pair<V3_عدد, V3_عدد>> runs{{range.lo, range.hi}};
                إذا (range.wildcard) {  // checkConstructedBins bounded the runs
                    runs.clear();
                    crossRangeRuns(range, v3Global.opt.coverageMaxBins(), runs);
                }
                لكل (ثابت std::pair<V3_عدد, V3_عدد>& لاڳيتو : runs) {
                    m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cpVarp, طريقو,
                                                       {new_قدر_Const(fl, لاڳيتو.پهريون, exprp),
                                                        new_قدر_Const(fl, لاڳيتو.ٻيو, exprp)})
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
        ثابت خودڪار bound_ويڪر
            = [&](ثابت Ast_ڳنڍ_اظهار* boundp) { أرجع unbounded(boundp) ? 0 : boundp->ويڪر(); };
        ثابت صحيح ويڪر = std::max({exprp->ويڪر(), bound_ويڪر(lowp), bound_ويڪر(highp)}) + 1;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(rangep, exprp->ويڪر(), exprp->isSigned(), ويڪر);
        AstVar* ثابت lop = constructor_عارضي(fl, اڳياڙي + "_lo", exprp->dtypep());
        lop->dtype_سيٽ_ٻٽو_انگ_Sized(ويڪر, VSigning::SIGNED);
        AstVar* ثابت hip = constructor_عارضي(fl, اڳياڙي + "_hi", lop->dtypep());
        ثابت خودڪار bound = [&](Ast_ڳنڍ_اظهار* boundp, ثابت V3_عدد& حد_بندي) -> Ast_ڳنڍ_اظهار* {
            إذا (unbounded(boundp)) {
                AstConst* ثابت limitp = جديد AstConst{fl, حد_بندي};
                limitp->dtypeFrom(lop);
                أرجع limitp;
            }
            Ast_ڳنڍ_اظهار* valuep = boundp->clone_وڻ(خطأ);
            // A value no wider than a signed coverpoint has its type (see crossRangeBound)
            إذا (exprp->isSigned() && boundp->ويڪر() <= exprp->ويڪر()) {
                valuep = resize_قدر(valuep, exprp->dtypep());
            }
            أرجع resize_قدر(valuep, lop->dtypep());
        };
        ثابت خودڪار ref = [&](AstVar* varp, VAccess access = VAccess::READ) {
            أرجع جديد AstVarRef{fl, varp, access};
        };
        m_constructorp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{fl, ref(lop, VAccess::WRITE), bound(lowp, domain.lo)});
        m_constructorp->جوڙيو_Stmtsp(
            جديد Ast_سونپيو{fl, ref(hip, VAccess::WRITE), irp ? bound(highp, domain.hi) : ref(lop)});
        AstConst* ثابت minp = جديد AstConst{fl, domain.lo};
        AstConst* ثابت maxp = جديد AstConst{fl, domain.hi};
        minp->dtypeFrom(lop);
        maxp->dtypeFrom(lop);
        AstCond* ثابت lowerp
            = جديد AstCond{fl, جديد AstLtS{fl, ref(lop), minp}, minp->clone_وڻ(خطأ), ref(lop)};
        AstCond* ثابت upperp
            = جديد AstCond{fl, جديد AstGtS{fl, ref(hip), maxp}, maxp->clone_وڻ(خطأ), ref(hip)};
        lowerp->dtypeFrom(lop);
        upperp->dtypeFrom(lop);
        m_constructorp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, ref(lop, VAccess::WRITE), lowerp});
        m_constructorp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, ref(hip, VAccess::WRITE), upperp});
        m_constructorp->جوڙيو_Stmtsp(جديد AstIf{fl, جديد AstLteS{fl, ref(lop), ref(hip)},
                                            شي_Call(fl, cpVarp, طريقو,
                                                     {resize_قدر(ref(lop), exprp->dtypep()),
                                                      resize_قدر(ref(hip), exprp->dtypep())})
                                                ->makeStmt()});
    }

    // The runtime index of the bin of a run holding the coverpoint value, which is in the run:
    // declared + (value - lo) / stride, capped at the last bin, which holds any remainder.
    ساكن Ast_ڳنڍ_اظهار* لاڳيتو_Bin_اشاريو(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, ثابت Bin_لاڳيتو& لاڳيتو) {
        إذا (لاڳيتو.m_ڳڻپ == 1) أرجع cnum(fl, لاڳيتو.m_declared);
        ثابت صحيح ويڪر = exprp->ويڪر();
        // A run spans at most 2^width values, so offsets in it are unsigned width-bit numbers
        Ast_ڳنڍ_اظهار* indexp
            = جديد AstSub{fl, exprp->clone_وڻ(خطأ), new_قدر_Const(fl, لاڳيتو.m_lo, exprp)};
        indexp->dtype_سيٽ_ٻٽو_انگ_Sized(ويڪر, VSigning::UNSIGNED);
        V3_عدد stride{fl, ويڪر, 0};
        stride.op_سونپيو(لاڳيتو.m_stride);
        إذا (stride.ڳڻپ_Ones() != 1) {
            indexp = جديد AstDiv{fl, indexp, جديد AstConst{fl, stride}};
        } وإلا إذا (!stride.isEqOne()) {
            indexp = جديد Ast_سرڪايو_R{fl, indexp, جديد AstConst{fl, stride.most_سيٽ_ٻٽو_انگ_P1() - 1}};
        }
        // Compare the run's values with those of count bins of stride values, without overflow
        ثابت صحيح ext_ويڪر = لاڳيتو.m_lo.ويڪر() + 1;
        V3_عدد lo{fl, ext_ويڪر, 0};
        lo.opExtendS(لاڳيتو.m_lo, لاڳيتو.m_lo.ويڪر());
        V3_عدد وسعت{fl, ext_ويڪر, 0};
        وسعت.opExtendS(لاڳيتو.m_hi, لاڳيتو.m_hi.ويڪر());
        وسعت.opSub(V3_عدد{وسعت}, lo);
        V3_عدد covered{fl, ext_ويڪر, 0};
        covered.op_سونپيو(لاڳيتو.m_stride);
        covered.opMul(V3_عدد{covered}, V3_عدد{fl, ext_ويڪر, لاڳيتو.m_ڳڻپ});
        V3_عدد hasRemainder{fl, 1, 0};
        إذا (!hasRemainder.opGte(وسعت, covered).isEqZero()) {
            AstConst* ثابت lastp = جديد AstConst{fl, V3_عدد{fl, ويڪر, لاڳيتو.m_ڳڻپ - 1}};
            indexp = جديد AstCond{fl, جديد AstGt{fl, indexp->clone_وڻ(خطأ), lastp},
                                 lastp->clone_وڻ(خطأ), indexp};
        }
        إذا (ويڪر < VL_IDATASIZE) {
            indexp = جديد AstExtend{fl, indexp, VL_IDATASIZE};
        } وإلا إذا (ويڪر > VL_IDATASIZE) {
            indexp = جديد AstSel{fl, indexp, 0, VL_IDATASIZE};
        }
        أرجع جديد Ast_جوڙيو{fl, cnum(fl, لاڳيتو.m_declared), indexp};
    }

    // Emit the sample() hit of a run of bins, whose code does not grow with its number of bins:
    //   if (iff && lo <= value && value <= hi) m_cp.incrementBin(<runBinIndex>);
    فراغ emit_لاڳيتو_ڪاميابي(AstCoverpoint* coverpointp, AstCoverBin* binp, AstVar* cpVarp,
                    Ast_ڳنڍ_اظهار* exprp, ثابت Bin_لاڳيتو& لاڳيتو) {
        File_سٽ* ثابت fl = binp->fileline();
        AstConst* ثابت lop = new_قدر_Const(fl, لاڳيتو.m_lo, exprp);
        Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
        إذا (لاڳيتو.m_lo.isCaseEq(لاڳيتو.m_hi)) {
            condp = جديد AstEq{fl, exprp->clone_وڻ(خطأ), lop};
        } وإلا {
            AstConst* ثابت hip = new_قدر_Const(fl, لاڳيتو.m_hi, exprp);
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڌڪو_Deletep(lop), lop);
            VL_DO_DANGLING(ڌڪو_Deletep(hip), hip);
        }
        emit_Conv_ڪاميابي_If(coverpointp, binp, cpVarp, لاڳيتو_Bin_اشاريو(fl, exprp, لاڳيتو), condp);
    }

    // Emit a transition bin's hit action into sample():
    //   if (cond) { m_cp.incrementBin/recordHit(idx); [illegal: $error; $stop] }
    // Used by the transition generators so a completed sequence records into the runtime bin.
    فراغ جوڙيو_Conv_Trans_ڪاميابي_If(AstCoverpoint* coverpointp, AstCoverBin* binp, ثابت Conv_Bin_هدف& tgt,
                           Ast_ڳنڍ_اظهار* condp) {
        File_سٽ* ثابت fl = binp->fileline();
        Ast_ڳنڍ* actionp = make_Runtime_Bin_ڪاميابي(fl, tgt);
        إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
            actionp->جوڙيو_ايندڙ(makeIllegalBinAction(
                fl, "Illegal transition bin " + binp->prettyNameQ() + " hit in coverpoint "
                        + coverpointp->prettyNameQ()));
        }
        UASSERT_OBJ(m_sampleFuncp, binp, "sample() CFunc not set for transition bin");
        جوڙيو_Sample_Stmt(جديد AstIf{fl, condp, actionp, خالي_اشارو});
    }

    // Route a coverpoint through a VlCoverpoint member: emit the member, its sample()
    // increments, the constructor configuration (init + namers), and registration.
    فراغ generateCoverpoint(AstCoverpoint* coverpointp, Ast_ڳنڍ_اظهار* exprp, صحيح at_گهٽ_کان_گهٽ_قدر) {
        File_سٽ* ثابت fl = coverpointp->fileline();
        ثابت منطقي dynamic = m_runtimePoints.ڳڻپ(coverpointp);
        UINFO(4, "  Generating VlCoverpoint member: " << coverpointp->name());

        // Size the hit list to the gen-time max bin overlap (1 unless cross-fed with
        // overlapping ranges), so no cross hit is ever dropped and storage is minimal.
        ثابت منطقي crossFed = m_crossed_Cp_نالا.ڳڻپ(coverpointp->name()) != 0;
        ثابت صحيح ڪاميابي_Bound = compute_ڪاميابي_List_Bound(coverpointp, exprp, crossFed);
        UINFO(6, "    Hit-list bound (max bin overlap) = " << ڪاميابي_Bound);
        AstVar* ثابت cpVarp = جديد AstVar{fl, VVarType::MEMBER, "__Vcp_" + coverpointp->name(),
                                          coverpointDType(fl, static_cast<uint32_t>(ڪاميابي_Bound))};
        m_covergroupp->جوڙيو_Membersp(cpVarp);
        m_cpVarMap[coverpointp->name()] = cpVarp;
        m_cpBins.emplace(cpVarp, CoverpointBins{});
        m_cpBins.at(cpVarp).exprp = exprp;
        m_cpBins.at(cpVarp).crossed = crossFed;
        // Create the runtime in the instance node first; everything below configures it.
        m_constructorp->جوڙيو_Stmtsp(make_شي_Create(fl, cpVarp, VCMethod::COVERGROUP_جوڙيو_COVERPOINT));
        generate_شي_وزن(fl, cpVarp, coverpointp->optionsp());

        // A cross reads this coverpoint's hit list, so clear it at the start of the
        // coverpoint's sample() contribution (before any incrementBin appends to it), even
        // when the iff guard disables sampling.
        إذا (crossFed) {
            UASSERT_OBJ(m_sampleFuncp, coverpointp, "sample() CFunc not set for clearHitList");
            m_sampleFuncp->جوڙيو_Stmtsp(
                شي_Call(fl, cpVarp, VCMethod::COVERGROUP_CLEAR_ڪاميابي_LIST)->makeStmt());
        }
        إذا (dynamic && coverpointHasStateExclusions(coverpointp)) {
            AstVar* ثابت excludedp
                = جديد AstVar{fl, VVarType::BLOCKTEMP,
                             "__VcpExcluded_" + sanitizeGeneratedName(coverpointp->name()),
                             coverpointp->ڳوليو_ٻٽو_انگ_DType()};
            excludedp->funcLocal(سچ);
            m_sampleFuncp->جوڙيو_Stmtsp(excludedp);
            AstCMethodHard* ثابت callp
                = شي_Call(fl, cpVarp,
                           exprp->isWide() ? VCMethod::COVERGROUP_قدر_EXCLUDED_W
                                           : VCMethod::COVERGROUP_قدر_EXCLUDED,
                           {exprp->clone_وڻ(خطأ)});
            callp->dtype_سيٽ_ٻٽو_انگ();
            جوڙيو_Sample_Stmt(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, excludedp, VAccess::WRITE}, callp});
            m_excludedVars.emplace(cpVarp, excludedp);
        }

        // Walk bins (non-default, then default), assigning sequential indices that match the
        // namer append order; emit sample increments and collect namer statements.  Constructed
        // bins follow them all, placed when the coverpoint is constructed.
        std::ويڪٽر<Ast_ڳنڍ_Stmt*> namerStmts;
        std::ويڪٽر<AstCoverBin*> defaultBins;
        std::ويڪٽر<AstCoverBin*> sizedBins;
        std::ويڪٽر<std::tuple<AstCoverBin*, uint32_t, Ast_ڳنڍ_اظهار*>> metadata;
        std::ويڪٽر<ثابت Bin_لاڳيتو*> لاڳيتو_Metadata;
        uint64_t idx = 0;  // Runtime index of the next bin; 32-bit once checked below
        لكل (Ast_ڳنڍ* binp = coverpointp->binsp(); binp; binp = binp->nextp()) {
            AstCoverBin* ثابت cbinp = VN_AS(binp, CoverBin);
            ثابت صحيح errorsBefore = dynamic ? V3Error::error_ڳڻپ() : 0;
            إذا (cbinp->binsType() == VCoverBinsType::BINS_DEFAULT) {
                defaultBins.ڌڪو_پٺ(cbinp);
                استمر;
            }
            إذا (isConstructedBins(cbinp)) {
                UASSERT_OBJ(dynamic, cbinp, "Constructed bins without value metadata");
                Bin_وسعت وسعت;
                وسعت.sized = static_cast<int32_t>(sizedBins.سائيز());
                m_cpBins.at(cpVarp).spans.emplace(cbinp->name(), وسعت);
                sizedBins.ڌڪو_پٺ(cbinp);
                استمر;
            }
            إذا (cbinp->transp()) {
                // Transition bin (incl. array transition 'bins t[] = (a=>b),(c=>d)' and
                // illegal_bins/ignore_bins transitions).  All sequences of one transition bin
                // share a bin name and merge in the coverage DB to a single point, so model
                // them as one runtime bin incremented by any matching sequence.  The sequence
                // matching is generated as a state machine, with the hit routed to this bin's
                // runtime slot.
                namerStmts.ڌڪو_پٺ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                ثابت Conv_Bin_هدف tgt{cpVarp, static_cast<uint32_t>(idx),
                                        cbinp->binsType().bin_Is_عام()};
                لكل (Ast_ڳنڍ* sp = cbinp->transp(); sp; sp = sp->nextp())
                    generate_Single_Transition_ڪوڊ(coverpointp, cbinp, exprp, tgt,
                                                 VN_AS(sp, Cover_Trans_سيٽ));
                إذا (dynamic && V3Error::error_ڳڻپ() == errorsBefore) {
                    metadata.emplace_پٺ(cbinp, static_cast<uint32_t>(idx), خالي_اشارو);
                }
                ++idx;
                استمر;
            }
            BinRuns plan;
            إذا (binRunsFor(cbinp, exprp, plan)) {
                // Array elements and automatic bins generate as runs, so neither sample() nor
                // the constructor grows with their number of bins.
                إذا (plan.unsupported) {  // bin ignored or invalid; reserve no slot
                    m_droppedBins[coverpointp].ڌڪو_پٺ(cbinp->name());
                    استمر;
                }
                CoverpointBins& bins = m_cpBins.at(cpVarp);
                ثابت uint32_t پهريون_قدر = bins.ڪل;
                ثابت uint32_t پهريون_Declared = static_cast<uint32_t>(idx);
                namerStmts.ڌڪو_پٺ(
                    makeNamer(cpVarp, cbinp, plan.ڳڻپ, پهريون_Declared, {}, plan.قدرون));
                لكل (Bin_لاڳيتو& لاڳيتو : plan.runs) {
                    لاڳيتو.m_declared = static_cast<uint32_t>(idx);
                    bins.runs.ڌڪو_پٺ(std::move(لاڳيتو));
                    ثابت Bin_لاڳيتو& stored = bins.runs.پٺ();
                    إذا (bins.crossed && cbinp->binsType().bin_Is_عام()) {
                        ثابت uint32_t پهريون = پهريون_قدر + stored.m_declared - پهريون_Declared;
                        لكل (uint32_t element = 0; element < stored.m_ڳڻپ; ++element) {
                            bins.قدرون[پهريون + element].runp = &stored;
                            bins.قدرون[پهريون + element].element = element;
                        }
                    }
                    إذا (!stored.m_خالي_جايون) {
                        emit_لاڳيتو_ڪاميابي(coverpointp, cbinp, cpVarp, exprp, stored);
                        إذا (dynamic && V3Error::error_ڳڻپ() == errorsBefore) {
                            لاڳيتو_Metadata.ڌڪو_پٺ(&stored);
                        }
                    }
                    idx += stored.m_ڳڻپ;
                }
                استمر;
            }
            إذا (cbinp->isArray()) {  // value array of a real coverpoint: b[0]..b[N-1]
                // Only integral coverpoints have runtime value metadata (m_runtimePoints)
                UASSERT_OBJ(!dynamic, cbinp, "Runtime value metadata for a real coverpoint");
                منطقي unsupported = خطأ;
                std::ويڪٽر<Ast_ڳنڍ_اظهار*> قدرون = ڪڍو_Array_قدرون(cbinp, exprp, unsupported);
                إذا (unsupported) {  // bin ignored (COVERIGN emitted); reserve no slot
                    m_droppedBins[coverpointp].ڌڪو_پٺ(cbinp->name());
                    استمر;
                }
                namerStmts.ڌڪو_پٺ(makeNamer(cpVarp, cbinp, static_cast<int64_t>(قدرون.سائيز()),
                                               static_cast<uint32_t>(idx), قدرون));
                لكل (Ast_ڳنڍ_اظهار* valuep : قدرون) {
                    // The cross selections of this covergroup still read the value.
                    m_detached_قدرون.ڌڪو_پٺ(valuep);
                    emit_Conv_ڪاميابي_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)),
                                  build_قدر_Condition(cbinp, exprp, valuep));
                    ++idx;
                }
            } وإلا {
                namerStmts.ڌڪو_پٺ(makeNamer(cpVarp, cbinp, -1, static_cast<uint32_t>(idx)));
                // buildBinCondition is null for 'ignore_bins = default' (no ranges); the bin
                // still gets a reserved slot (recorded, never incremented).
                إذا (Ast_ڳنڍ_اظهار* ثابت condp = buildBinCondition(cbinp, exprp))
                    emit_Conv_ڪاميابي_If(coverpointp, cbinp, cpVarp,
                                  cnum(cbinp->fileline(), static_cast<uint32_t>(idx)), condp);
                إذا (dynamic && V3Error::error_ڳڻپ() == errorsBefore) {
                    metadata.emplace_پٺ(cbinp, static_cast<uint32_t>(idx), خالي_اشارو);
                }
                ++idx;
            }
        }
        // A cross selecting an ignored bins declaration selects no bins
        لكل (ثابت std::string& name : m_droppedBins[coverpointp]) {
            m_cpBins.at(cpVarp).spans.emplace(name, Bin_وسعت{});
        }
        // A value a constructed bin holds is no default bin's; only sampling tells which do
        AstVar* sizedMatchedp = خالي_اشارو;
        إذا (!defaultBins.خالي_جايون()
            && std::any_جو(sizedBins.begin(), sizedBins.پڇاڙي(), [](ثابت AstCoverBin* binp) {
                   أرجع binp->binsType().bin_Is_عام();
               })) {
            sizedMatchedp = جديد AstVar{fl, VVarType::BLOCKTEMP,
                                       "__VcpSized_" + sanitizeGeneratedName(coverpointp->name()),
                                       coverpointp->ڳوليو_ٻٽو_انگ_DType()};
            sizedMatchedp->funcLocal(سچ);
            m_sampleFuncp->جوڙيو_Stmtsp(sizedMatchedp);
            جوڙيو_Sample_Stmt(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, sizedMatchedp, VAccess::WRITE},
                                        جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}}});
        }
        لكل (uint32_t sized = 0; sized < sizedBins.سائيز(); ++sized) {
            emitSizedSample(coverpointp, sizedBins[sized], cpVarp, exprp, sized, sizedMatchedp);
        }
        لكل (AstCoverBin* ثابت defBinp : defaultBins) {
            File_سٽ* ثابت dfl = defBinp->fileline();
            namerStmts.ڌڪو_پٺ(makeNamer(cpVarp, defBinp, -1, static_cast<uint32_t>(idx)));
            Ast_ڳنڍ_اظهار* condp = buildDefaultCondition(coverpointp, exprp, dfl);
            إذا (sizedMatchedp) {
                condp = جديد AstLogAnd{
                    dfl, جديد AstNot{dfl, جديد AstVarRef{dfl, sizedMatchedp, VAccess::READ}}, condp};
            }
            emit_Conv_ڪاميابي_If(coverpointp, defBinp, cpVarp, cnum(dfl, static_cast<uint32_t>(idx)),
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
            جوڙيو_Sample_Stmt(
                جديد Ast_سونپيو{coverpointp->fileline(),
                              جديد AstVarRef{prevVarp->fileline(), prevVarp, VAccess::WRITE},
                              exprp->clone_وڻ(خطأ)});
        }

        // Constructor: init (allocates), namers, then registration (under --coverage).
        // Under --protect-ids the hierarchy and page string reach the coverage database
        // verbatim, so obfuscate them like line/toggle points (per-word hierarchy, whole-
        // unit page).  No-ops when --protect-ids is off.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_لفظ_If(m_covergroupName + "." + coverpointp->name(), prot);
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cpVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_گهٽ_کان_گهٽ_قدر)),
                      cnum(fl, static_cast<uint32_t>(idx))})
                ->makeStmt());
        لكل (Ast_ڳنڍ_Stmt* ثابت ns : namerStmts) m_constructorp->جوڙيو_Stmtsp(ns);
        إذا (dynamic) {
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_TYPE,
                         {cnum(fl, exprp->ويڪر()), cnum(fl, exprp->isSigned())})
                    ->makeStmt());
            قدر_Lists lists;
            لكل (ثابت خودڪار& داخلا_2 : metadata) {
                collect_قدر_Metadata(lists, exprp, std::get<0>(داخلا_2), std::get<1>(داخلا_2),
                                     std::get<2>(داخلا_2));
            }
            لكل (ثابت Bin_لاڳيتو* ثابت runp : لاڳيتو_Metadata) collect_لاڳيتو_Metadata(lists, exprp, *runp);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RANGES, lists.m_ranges);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_RUNS, lists.m_runs);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_PATTERNS, lists.m_patterns);
            emit_قدر_List(fl, cpVarp, VCMethod::COVERGROUP_قدر_TRANSITIONS, lists.m_transitions);
            لكل (AstCoverBin* ثابت binp : sizedBins) {
                generateConstructedBins(coverpointp, binp, cpVarp, exprp);
            }
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cpVarp, VCMethod::COVERGROUP_قدر_FINALIZE)->makeStmt());
        }
        إذا (v3Global.opt.coverage()) {
            ثابت std::string صفحو
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cpVarp, VCMethod::COVERGROUP_رجسٽر_BINS,
                         {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                          ctext(fl, quoted(صفحو)),
                          cnum(fl, شي_Database_وزن(coverpointp->optionsp())),
                          cnum(fl, m_cg_Type_وزن)})
                    ->makeStmt());
        }
    }

    // Generate state machine code for multi-value transition sequences
    // Handles transitions like (1 => 2 => 3 => 4)
    فراغ generate_Multi_قدر_Transition_ڪوڊ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                          Ast_ڳنڍ_اظهار* exprp, ثابت Conv_Bin_هدف& tgt,
                                          ثابت std::ويڪٽر<Ast_Cover_Trans_شي*>& شيون) {
        UINFO(4, "    Generating multi-value transition state machine for: " << binp->name());
        UINFO(4, "      Sequence length: " << شيون.سائيز() << " items");

        // Create state position variable
        AstVar* ثابت stateVarp = createSequenceStateVar(coverpointp, binp);

        // Build case statement with N cases (one for each state 0 to N-1)
        // State 0: Not started, looking for first item
        // State 1 to N-1: In progress, looking for next item

        AstCase* ثابت casep
            = جديد AstCase{binp->fileline(), VCaseType::CT_CASE,
                          جديد AstVarRef{stateVarp->fileline(), stateVarp, VAccess::READ}, خالي_اشارو};

        // Generate each case item in the switch statement
        لكل (سائيز_t state = 0; state < شيون.سائيز(); ++state) {
            Ast_Case_شي* caseItemp = generateTransitionStateCase(coverpointp, binp, exprp, tgt,
                                                                 stateVarp, شيون, state);
            casep->جوڙيو_Itemsp(caseItemp);
        }

        // Add default case (reset to state 0) to prevent CASEINCOMPLETE warnings,
        // since the state variable is wider than the number of valid states.
        Ast_Case_شي* ثابت defaultItemp = جديد Ast_Case_شي{
            binp->fileline(), خالي_اشارو,
            جديد Ast_سونپيو{binp->fileline(),
                          جديد AstVarRef{binp->fileline(), stateVarp, VAccess::WRITE},
                          جديد AstConst{binp->fileline(), AstConst::Widthed_قدر{}, 8, 0}}};
        casep->جوڙيو_Itemsp(defaultItemp);

        جوڙيو_Sample_Stmt(casep);
        UINFO(4, "      Successfully added multi-value transition state machine");
    }

    // Generate code for a single state in the transition state machine
    // Returns the case item for this state
    Ast_Case_شي* generateTransitionStateCase(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                             Ast_ڳنڍ_اظهار* exprp, ثابت Conv_Bin_هدف& tgt,
                                             AstVar* stateVarp,
                                             ثابت std::ويڪٽر<Ast_Cover_Trans_شي*>& شيون,
                                             سائيز_t state) {
        File_سٽ* ثابت fl = binp->fileline();

        // Build condition for current value matching expected item at this state
        Ast_ڳنڍ_اظهار* ثابت ميلاپ_Condp = build_Transition_شي_Condition(شيون[state], exprp);

        Ast_ڳنڍ_Stmt* ميلاپ_Actionp = خالي_اشارو;

        إذا (state == شيون.سائيز() - 1) {
            // Last state: sequence complete!  Record the hit in the runtime VlCoverpoint.
            ميلاپ_Actionp = make_Runtime_Bin_ڪاميابي(fl, tgt);

            // For illegal_bins, add error message
            إذا (binp->binsType() == VCoverBinsType::BINS_ILLEGAL) {
                ثابت string err_نياپو = "Illegal transition bin " + binp->prettyNameQ()
                                      + " hit in coverpoint " + coverpointp->prettyNameQ();
                ميلاپ_Actionp = ميلاپ_Actionp->جوڙيو_ايندڙ(makeIllegalBinAction(fl, err_نياپو));
            }

            // Reset state to 0
            ميلاپ_Actionp = ميلاپ_Actionp->جوڙيو_ايندڙ(
                جديد Ast_سونپيو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                              جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}});
        } وإلا {
            // Intermediate state: advance to next state
            ميلاپ_Actionp = جديد Ast_سونپيو{
                fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state + 1)}};
        }

        // Build restart logic: check if current value matches first item
        // If so, restart sequence from state 1 (even if we're in middle of sequence)
        Ast_ڳنڍ_Stmt* no_ميلاپ_Actionp = خالي_اشارو;
        إذا (state > 0) {
            // Check if current value matches first item (restart condition)
            Ast_ڳنڍ_اظهار* ثابت restartCondp = build_Transition_شي_Condition(شيون[0], exprp);

            UASSERT_OBJ(restartCondp, شيون[0],
                        "buildTransitionItemCondition returned nullptr for restart");

            // Restart to state 1
            Ast_ڳنڍ_Stmt* restartActionp
                = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 1}};

            // Reset to state 0 (else branch)
            Ast_ڳنڍ_Stmt* resetActionp
                = جديد Ast_سونپيو{fl, جديد AstVarRef{fl, stateVarp, VAccess::WRITE},
                                جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, 0}};

            no_ميلاپ_Actionp = جديد AstIf{fl, restartCondp, restartActionp, resetActionp};
        }
        // For state 0, no action needed if no match (stay in state 0)

        // Combine into if-else
        Ast_ڳنڍ_Stmt* ثابت stmtp = جديد AstIf{fl, ميلاپ_Condp, ميلاپ_Actionp, no_ميلاپ_Actionp};

        // Create case item for this state value
        Ast_Case_شي* ثابت caseItemp = جديد Ast_Case_شي{
            fl, جديد AstConst{fl, AstConst::Widthed_قدر{}, 8, static_cast<uint32_t>(state)},
            stmtp};

        أرجع caseItemp;
    }

    // Create: $error(msg); $stop;  Used when an illegal bin is hit.
    Ast_ڳنڍ_Stmt* makeIllegalBinAction(File_سٽ* fl, ثابت string& err_نياپو) {
        AstDisplay* ثابت errorp
            = جديد AstDisplay{fl, VDisplayType::DT_ERROR, err_نياپو, خالي_اشارو, خالي_اشارو};
        errorp->fmtp()->timeunit(m_covergroupp->timeunit());
        static_cast<Ast_ڳنڍ*>(errorp)->جوڙيو_ايندڙ(جديد AstStop{fl, سچ});
        أرجع errorp;
    }

    // Preserve the coverpoint's width and signedness after V3Width.
    ساكن AstConst* new_قدر_Const(File_سٽ* fl, ثابت V3_عدد& قدر, ثابت Ast_ڳنڍ_اظهار* exprp) {
        V3_عدد narrowed{fl, exprp->ويڪر(), 0};
        narrowed.op_سونپيو(قدر);
        AstConst* ثابت constp = جديد AstConst{fl, narrowed};
        constp->dtypeFrom(exprp);
        أرجع constp;
    }

    // A real copy of a real or integral range bound, for comparing with a real coverpoint.
    ساكن AstConst* newRealConst(AstConst* constp) {
        إذا (constp->انگ().isDouble()) أرجع constp->clone_وڻ(خطأ);
        V3_عدد real{&constp->انگ(), 64};
        real.opIToRD(constp->انگ(), constp->isSigned());
        أرجع جديد AstConst{constp->fileline(), real};
    }

    // Clone a constant node, widening to targetWidth if needed (zero-extend).
    // Used to ensure comparisons use matching widths after V3Width has run.
    ساكن AstConst* widenConst(File_سٽ* fl, AstConst* constp, صحيح هدف_ويڪر) {
        إذا (constp->ويڪر() == هدف_ويڪر) أرجع constp->clone_وڻ(خطأ);
        V3_عدد انگ{fl, هدف_ويڪر, 0};
        انگ.op_سونپيو(constp->انگ());
        أرجع جديد AstConst{fl, انگ};
    }

    // Build a range condition: minp <= exprp <= maxp.
    // Uses signed comparisons if exprp is signed; omits trivially-true domain bounds.
    // All arguments are non-owning; clones exprp/minp/maxp as needed.
    Ast_ڳنڍ_اظهار* makeRangeCondition(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ_اظهار* minp,
                                    Ast_ڳنڍ_اظهار* maxp) {
        ثابت صحيح اظهار_ويڪر = exprp->ويڪر_Min();
        AstConst* ثابت minConstp = VN_AS(minp, Const);
        AstConst* ثابت maxConstp = VN_AS(maxp, Const);
        إذا (exprp->isDouble()) {
            // A real coverpoint has no finite domain bounds to omit.
            أرجع جديد AstAnd{fl,
                              جديد AstGteD{fl, exprp->clone_وڻ(خطأ), newRealConst(minConstp)},
                              جديد AstLteD{fl, exprp->clone_وڻ(خطأ), newRealConst(maxConstp)}};
        }
        // Widen constants to match expression width so post-V3Width nodes use correct macros
        AstConst* ثابت minWidep = widenConst(fl, minConstp, اظهار_ويڪر);
        AstConst* ثابت maxWidep = widenConst(fl, maxConstp, اظهار_ويڪر);
        V3_عدد minimum{fl, اظهار_ويڪر, 0};
        V3_عدد وڌ_کان_وڌ{fl, اظهار_ويڪر, 0};
        وڌ_کان_وڌ.سيٽ_All_Bits1();
        إذا (exprp->isSigned()) {
            minimum.سيٽ_ٻٽو_انگ(اظهار_ويڪر - 1, 1);
            وڌ_کان_وڌ.سيٽ_ٻٽو_انگ(اظهار_ويڪر - 1, 0);
        }
        Ast_ڳنڍ_اظهار* lowerp = خالي_اشارو;
        Ast_ڳنڍ_اظهار* upperp = خالي_اشارو;
        إذا (minWidep->انگ().isCaseEq(minimum)) {
            VL_DO_DANGLING(ڌڪو_Deletep(minWidep), minWidep);
        } وإلا {
            lowerp = exprp->isSigned() ? static_cast<Ast_ڳنڍ_اظهار*>(
                                             جديد AstGteS{fl, exprp->clone_وڻ(خطأ), minWidep})
                                       : static_cast<Ast_ڳنڍ_اظهار*>(
                                             جديد AstGte{fl, exprp->clone_وڻ(خطأ), minWidep});
        }
        إذا (maxWidep->انگ().isCaseEq(وڌ_کان_وڌ)) {
            VL_DO_DANGLING(ڌڪو_Deletep(maxWidep), maxWidep);
        } وإلا {
            upperp = exprp->isSigned() ? static_cast<Ast_ڳنڍ_اظهار*>(
                                             جديد AstLteS{fl, exprp->clone_وڻ(خطأ), maxWidep})
                                       : static_cast<Ast_ڳنڍ_اظهار*>(
                                             جديد AstLte{fl, exprp->clone_وڻ(خطأ), maxWidep});
        }
        إذا (lowerp && upperp) أرجع جديد AstAnd{fl, lowerp, upperp};
        إذا (lowerp) أرجع lowerp;
        إذا (upperp) أرجع upperp;
        أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}};
    }

    // Build a one-sided comparison for an open-ended bin range whose other bound is '$'.
    // '$' denotes the coverpoint domain extreme, so {[lo:$]} == (expr >= lo) and
    // {[$:hi]} == (expr <= hi).
    Ast_ڳنڍ_اظهار* makeOpenRangeCondition(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, AstConst* boundp,
                                        منطقي is_هيٺاهون_Bound) {
        AstConst* ثابت widep = widenConst(fl, boundp, exprp->ويڪر_Min());
        إذا (is_هيٺاهون_Bound) {
            إذا (exprp->isSigned()) أرجع جديد AstGteS{fl, exprp->clone_وڻ(خطأ), widep};
            أرجع جديد AstGte{fl, exprp->clone_وڻ(خطأ), widep};
        }
        إذا (exprp->isSigned()) أرجع جديد AstLteS{fl, exprp->clone_وڻ(خطأ), widep};
        أرجع جديد AstLte{fl, exprp->clone_وڻ(خطأ), widep};
    }

    // Build condition for a single transition item.
    // Returns expression that checks if exprp matches the item's value/range list.
    // Overload for when the expression is a variable read -- creates and manages the VarRef
    // internally, so callers don't need to construct a temporary node.
    Ast_ڳنڍ_اظهار* build_Transition_شي_Condition(Ast_Cover_Trans_شي* itemp, AstVar* varp) {
        Ast_ڳنڍ_اظهار* varRefp = جديد AstVarRef{varp->fileline(), varp, VAccess::READ};
        Ast_ڳنڍ_اظهار* ثابت condp = build_Transition_شي_Condition(itemp, varRefp);
        VL_DO_DANGLING(ڌڪو_Deletep(varRefp), varRefp);
        أرجع condp;
    }

    // Non-owning: exprp is cloned internally; caller retains ownership of exprp.
    Ast_ڳنڍ_اظهار* build_Transition_شي_Condition(Ast_Cover_Trans_شي* itemp, Ast_ڳنڍ_اظهار* exprp) {
        Ast_ڳنڍ_اظهار* condp = خالي_اشارو;

        لكل (Ast_ڳنڍ* valp = itemp->valuesp(); valp; valp = valp->nextp()) {
            Ast_ڳنڍ_اظهار* singleCondp = خالي_اشارو;
            valp = V3Const::constifyEdit(valp);
            AstConst* ثابت constp = VN_CAST(valp, Const);
            إذا (!constp) {
                valp->v3error("Non-constant expression in transition bin; "
                              "values must be constants (IEEE 1800-2023 19.5)");
                أرجع جديد AstConst{valp->fileline(), AstConst::ٻٽو_انگ_False_Erroring{}};
            }
            singleCondp
                = جديد AstEq{constp->fileline(), exprp->clone_وڻ(خطأ), constp->clone_وڻ(خطأ)};
            إذا (condp) {
                condp = جديد AstOr{itemp->fileline(), condp, singleCondp};
            } وإلا {
                condp = singleCondp;
            }
        }

        أرجع condp;
    }

    // Generate code for a single transition sequence (used by both regular and array bins)
    فراغ generate_Single_Transition_ڪوڊ(AstCoverpoint* coverpointp, AstCoverBin* binp,
                                      Ast_ڳنڍ_اظهار* exprp, ثابت Conv_Bin_هدف& tgt,
                                      Ast_Cover_Trans_سيٽ* transSetp) {
        UINFO(4, "      Generating code for transition sequence");

        // Get or create previous value variable
        AstVar* ثابت prevVarp = create_Prev_قدر_Var(coverpointp, exprp);

        UASSERT_OBJ(
            transSetp, binp,
            "Transition bin has no transition set (transp() was checked before calling this)");

        // Get transition items (the sequence: item1 => item2 => item3)
        std::ويڪٽر<Ast_Cover_Trans_شي*> شيون;
        لكل (Ast_ڳنڍ* itemp = transSetp->itemsp(); itemp; itemp = itemp->nextp())
            شيون.ڌڪو_پٺ(VN_AS(itemp, Cover_Trans_شي));

        إذا (شيون.خالي_جايون()) {
            binp->v3error("Transition set without items");
            أرجع;
        }

        إذا (شيون.سائيز() == 1) {
            // Single item transition not valid (need at least 2 values for =>)
            binp->v3error("Transition requires at least two values");
            أرجع;
        } وإلا إذا (شيون.سائيز() == 2) {
            // Simple two-value transition: (val1 => val2)
            // Use optimized direct comparison (no state machine needed)
            Ast_ڳنڍ_اظهار* ثابت cond1p = build_Transition_شي_Condition(شيون[0], prevVarp);
            Ast_ڳنڍ_اظهار* ثابت cond2p = build_Transition_شي_Condition(شيون[1], exprp);

            // Combine: prev matches val1 AND current matches val2
            Ast_ڳنڍ_اظهار* ڀريل_Condp = جديد AstAnd{binp->fileline(), cond1p, cond2p};

            جوڙيو_Conv_Trans_ڪاميابي_If(coverpointp, binp, tgt, ڀريل_Condp);

            UINFO(4, "        Successfully added 2-value transition if statement");
        } وإلا {
            // Multi-value sequence (a => b => c => ...)
            // Use state machine to track position in sequence
            generate_Multi_قدر_Transition_ڪوڊ(coverpointp, binp, exprp, tgt, شيون);
        }
    }

    // Append a "{ VlCoverpoint* __Vcx_cps[] = {cp0, cp1, ...}; <call> }" statement.  The brace
    // and the temporary array stay literal text -- a CMethodHard is one call, not a block --
    // but callp itself carries the member, method and '->'.  Construction only: init() copies
    // the array into the cross, so sample() reads it from there and needs no array at all.
    AstCStmt* makeCrossCpsCall(File_سٽ* fl, ثابت std::ويڪٽر<AstVar*>& cpVars,
                               AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جوڙيو("{ VlCoverpoint* __Vcx_cps[] = {");
        لكل (سائيز_t d = 0; d < cpVars.سائيز(); ++d) {
            إذا (d != 0) cs->جوڙيو(", ");
            cs->جوڙيو(memberRef(fl, cpVars[d]));
        }
        cs->جوڙيو("}; ");
        cs->جوڙيو(callp);
        cs->جوڙيو("; }");
        أرجع cs;
    }

    // Assign the per-bin flags individually: one-bit SV results have integer C++ storage types,
    // which may narrow in a bool initializer list but convert implicitly in assignments.
    AstCStmt* makeCrossIffsCall(File_سٽ* fl, ثابت std::ويڪٽر<AstCoverCrossBin*>& bins,
                                AstCMethodHard* callp) {
        AstCStmt* ثابت cs = جديد AstCStmt{fl};
        cs->جوڙيو("{ bool __Vcx_iffs[" + cvt_منزل_Str(bins.سائيز()) + "]; ");
        لكل (سائيز_t i = 0; i < bins.سائيز(); ++i) {
            ثابت AstCoverCrossBin* ثابت binp = bins[i];
            cs->جوڙيو("__Vcx_iffs[" + cvt_منزل_Str(i) + "] = ");
            cs->جوڙيو(binp->iffp() ? binp->iffp()->clone_وڻ(خطأ)
                                 : جديد AstConst{fl, AstConst::ٻٽو_انگ_True{}});
            cs->جوڙيو("; ");
        }
        cs->جوڙيو(callp);
        cs->جوڙيو("; }");
        أرجع cs;
    }

    استعمال CrossSelection = std::ويڪٽر<uint64_t>;
    بنية ResolvedCrossBin final {
        AstCoverCrossBin* binp;
        CrossSelection selection;
    };
    بنية CrossLayout final {
        uint32_t tuples = 0;
        uint32_t autoBins = 0;
        uint64_t bin_لفظ = 0;
        منطقي valid = سچ;
        std::ويڪٽر<ResolvedCrossBin> bins;
    };
    بنية CrossSelectionContext final {
        AstCoverCross* crossp;  // Cross whose tuple space is being selected
        ثابت std::ويڪٽر<AstVar*>& cpVars;  // Feeding coverpoints in dimension order
        ثابت std::map<std::string, uint32_t>& dimensions;  // Coverpoint name -> dimension
        uint32_t tuples;  // Size of the Cartesian product
        std::ويڪٽر<uint32_t> strides;  // Flat-index stride per dimension
        منطقي valid = سچ;  // False if this explicit bin cannot be implemented
    };
    بنية Cross_Binsof_هدف final {
        ثابت CoverpointBins* m_binsp = خالي_اشارو;  // Declared bins; null after a reported error
        uint32_t m_dimension = 0;  // Coverpoint index within the cross
        uint32_t m_پهريون = 0;  // First declared normal bin in the selected span
        uint32_t m_ڳڻپ = 0;  // Number of declared normal bins in the selected span
        uint32_t m_declared_پهريون = 0;  // First runtime bin index of the selected span
        uint32_t m_declared_پڇاڙي = UINT32_MAX;  // One past the last runtime bin index
        int32_t m_sized = -1;  // Index of the sized array holding the selected bins; or -1
    };
    بنية Cross_قدر_Range final {
        V3_عدد lo;  // Inclusive lower bound, sign-extended to the comparison width
        V3_عدد hi;  // Inclusive upper bound
        V3_عدد نمونو_2;  // Allowed bit values; all X for an ordinary interval
        منطقي wildcard = خطأ;  // A wildcard singleton rather than an exact four-state value

        Cross_قدر_Range(Ast_ڳنڍ* nodep, صحيح ويڪر)
            : lo{nodep, ويڪر}
            , hi{nodep, ويڪر}
            , نمونو_2{nodep, ويڪر} {
            نمونو_2.سيٽ_All_بٽ_X();
        }
    };

    ساكن std::ويڪٽر<Ast_ڳنڍ*> cross_Bin_قدرون(ثابت Cross_Bin_قدرون& bin) {
        إذا (bin.valuep) أرجع {bin.valuep};
        std::ويڪٽر<Ast_ڳنڍ*> قدرون;
        إذا (bin.binp->transp()) {
            // IEEE 1800-2023 19.6.1: binsof uses the last value of each transition.
            لكل (Ast_ڳنڍ* setp = bin.binp->transp(); setp; setp = setp->nextp()) {
                Ast_Cover_Trans_شي* lastp = VN_AS(setp, Cover_Trans_سيٽ)->itemsp();
                طالما (lastp->nextp()) lastp = VN_AS(lastp->nextp(), Cover_Trans_شي);
                لكل (Ast_ڳنڍ* valuep = lastp->valuesp(); valuep; valuep = valuep->nextp()) {
                    قدرون.ڌڪو_پٺ(valuep);
                }
            }
        } وإلا {
            لكل (Ast_ڳنڍ* valuep = bin.binp->rangesp(); valuep; valuep = valuep->nextp()) {
                قدرون.ڌڪو_پٺ(valuep);
            }
        }
        أرجع قدرون;
    }

    // The values of the element-th bin of a run, sign-extended to 'width'
    ساكن Cross_قدر_Range لاڳيتو_Bin_Range(Ast_ڳنڍ* nodep, ثابت Bin_لاڳيتو& لاڳيتو, uint32_t element,
                                       صحيح ويڪر) {
        ثابت صحيح runw = لاڳيتو.m_lo.ويڪر();
        V3_عدد وٿي{nodep, runw};
        وٿي.opMul(لاڳيتو.m_stride, V3_عدد{nodep, runw, element});
        V3_عدد lo{nodep, runw};
        lo.op_جوڙيو(لاڳيتو.m_lo, وٿي);
        V3_عدد hi = لاڳيتو.m_hi;
        إذا (element + 1 < لاڳيتو.m_ڳڻپ) {
            وٿي.opSub(لاڳيتو.m_stride, V3_عدد{nodep, runw, 1});
            hi.op_جوڙيو(lo, وٿي);
        }
        Cross_قدر_Range range{nodep, ويڪر};
        range.lo.opExtendS(lo, runw);
        range.hi.opExtendS(hi, runw);
        أرجع range;
    }

    ساكن صحيح cross_Range_ويڪر(Ast_ڳنڍ* nodep) {
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            أرجع std::max(rangep->lhsp()->ويڪر(), rangep->rhsp()->ويڪر());
        }
        أرجع nodep->ويڪر();
    }

    ساكن منطقي cross_قدر_Less(ثابت V3_عدد& lhs, ثابت V3_عدد& rhs) {
        V3_عدد نتيجو{&lhs};
        أرجع !نتيجو.opLtS(lhs, rhs).isEqZero();
    }

    // Round a real bound inward to the nearest coverpoint value (IEEE 1800-2023 19.5.7).  Sets
    // 'empty' if the domain has no value on the bound's side.
    ساكن فراغ crossRealBound(ثابت AstConst* constp, Ast_ڳنڍ_اظهار* exprp, منطقي مٿانهون,
                               ثابت Cross_قدر_Range& domain, V3_عدد& نتيجو, منطقي& خالي_جايون) {
        ثابت مزدوج قدر = constp->انگ().منزل_Double();
        ثابت مزدوج bound = مٿانهون ? std::floor(قدر) : std::ceil(قدر);
        // Powers of two are exact, so the integral bound compares exactly with the domain edges.
        ثابت مزدوج حد_بندي
            = std::ldexp(1.0, exprp->isSigned() ? exprp->ويڪر() - 1 : exprp->ويڪر());
        ثابت مزدوج minimum = exprp->isSigned() ? -حد_بندي : 0.0;
        إذا (std::isnan(bound) || (مٿانهون ? bound < minimum : bound >= حد_بندي)) {
            خالي_جايون = سچ;
            نتيجو = domain.lo;
        } وإلا إذا (bound < minimum) {
            نتيجو = domain.lo;
        } وإلا إذا (bound >= حد_بندي) {
            نتيجو = domain.hi;
        } وإلا {
            V3_عدد real{&نتيجو, 64};
            real.سيٽ_Double(bound);
            نتيجو.opRToIRoundS(real);
        }
    }

    ساكن منطقي crossRangeBound(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* exprp, منطقي مٿانهون, منطقي bin_قدر,
                                ثابت Cross_قدر_Range& domain, V3_عدد& نتيجو, منطقي& خالي_جايون) {
        إذا (VN_IS(nodep, Unbounded)) {
            V3_عدد حد_بندي{nodep, exprp->ويڪر()};
            إذا (مٿانهون) حد_بندي.سيٽ_All_Bits1();
            إذا (exprp->isSigned()) {
                حد_بندي.سيٽ_ٻٽو_انگ(exprp->ويڪر() - 1, !مٿانهون);
                نتيجو.opExtendS(حد_بندي, حد_بندي.ويڪر());
            } وإلا {
                نتيجو.op_سونپيو(حد_بندي);
            }
            أرجع سچ;
        }
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp) أرجع خطأ;
        إذا (constp->انگ().isDouble()) {
            crossRealBound(constp, exprp, مٿانهون, domain, نتيجو, خالي_جايون);
            أرجع سچ;
        }
        إذا (constp->انگ().isString()) أرجع خطأ;
        إذا (bin_قدر && exprp->isSigned() && constp->ويڪر() <= exprp->ويڪر()) {
            // Bin bit patterns use the coverpoint's effective type (IEEE 1800-2023 19.5.7).
            // Wider values and intersect filters retain their values for domain clipping.
            V3_عدد قدر{nodep, exprp->ويڪر()};
            إذا (constp->isSigned()) {
                قدر.opExtendS(constp->انگ(), constp->ويڪر());
            } وإلا {
                قدر.op_سونپيو(constp->انگ());
            }
            نتيجو.opExtendS(قدر, قدر.ويڪر());
        } وإلا إذا (constp->isSigned()) {
            نتيجو.opExtendS(constp->انگ(), constp->ويڪر());
        } وإلا {
            نتيجو.op_سونپيو(constp->انگ());
        }
        أرجع سچ;
    }

    ساكن Cross_قدر_Range cross_قدر_Domain(Ast_ڳنڍ* nodep, صحيح قدر_ويڪر, منطقي isSigned,
                                            صحيح ويڪر) {
        Cross_قدر_Range domain{nodep, ويڪر};
        V3_عدد lo{nodep, قدر_ويڪر};
        V3_عدد hi{nodep, قدر_ويڪر};
        hi.سيٽ_All_Bits1();
        إذا (isSigned) {
            lo.سيٽ_ٻٽو_انگ(قدر_ويڪر - 1, 1);
            hi.سيٽ_ٻٽو_انگ(قدر_ويڪر - 1, 0);
            domain.lo.opExtendS(lo, قدر_ويڪر);
            domain.hi.opExtendS(hi, قدر_ويڪر);
        } وإلا {
            domain.lo.op_سونپيو(lo);
            domain.hi.op_سونپيو(hi);
        }
        أرجع domain;
    }

    ساكن فراغ intersectCrossRange(Cross_قدر_Range& range, ثابت Cross_قدر_Range& other) {
        إذا (cross_قدر_Less(range.lo, other.lo)) range.lo = other.lo;
        إذا (cross_قدر_Less(other.hi, range.hi)) range.hi = other.hi;
    }

    ساكن منطقي cross_قدر_Range(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* exprp, منطقي bin_قدر, منطقي wildcard,
                                ثابت Cross_قدر_Range& domain, Cross_قدر_Range& range) {
        منطقي خالي_جايون = خطأ;
        ثابت AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (ثابت AstInsideRange* ثابت rangep = VN_CAST(nodep, InsideRange)) {
            إذا (!crossRangeBound(rangep->lhsp(), exprp, خطأ, bin_قدر, domain, range.lo, خالي_جايون)
                || !crossRangeBound(rangep->rhsp(), exprp, سچ, bin_قدر, domain, range.hi, خالي_جايون)
                || range.lo.isFourState() || range.hi.isFourState()) {
                أرجع خطأ;
            }
        } وإلا إذا (constp && constp->انگ().isDouble()) {
            // A real value participates only if integral; it has no wildcard bits.
            crossRealBound(constp, exprp, خطأ, domain, range.lo, خالي_جايون);
            crossRealBound(constp, exprp, سچ, domain, range.hi, خالي_جايون);
        } وإلا {
            إذا (!crossRangeBound(nodep, exprp, خطأ, bin_قدر, domain, range.lo, خالي_جايون)) {
                أرجع خطأ;
            }
            إذا (bin_قدر && exprp->isSigned() && constp && !constp->isSigned()
                && constp->ويڪر() > exprp->ويڪر()) {
                منطقي representable = سچ;
                لكل (صحيح ٻٽو_انگ_2 = exprp->ويڪر(); ٻٽو_انگ_2 < constp->ويڪر(); ++ٻٽو_انگ_2) {
                    representable &= !constp->انگ().ٻٽو_انگ_Is1(ٻٽو_انگ_2);
                }
                إذا (representable) {
                    V3_عدد قدر{nodep, exprp->ويڪر()};
                    قدر.op_سونپيو(constp->انگ());
                    range.lo.opExtendS(قدر, قدر.ويڪر());
                }
            }
            range.hi = range.lo;
            range.wildcard = wildcard;
            إذا (wildcard) {
                range.نمونو_2 = range.lo;
                range.lo = domain.lo;
                range.hi = domain.hi;
                إذا (constp && constp->isSigned()) {
                    // Replicated X sign bits are correlated, not independent wildcards.
                    // The source domain preserves expansion-before-casting (19.5.7).
                    intersectCrossRange(
                        range, cross_قدر_Domain(nodep, constp->ويڪر(), سچ, domain.lo.ويڪر()));
                }
            }
        }
        إذا (خالي_جايون) {
            range.lo = domain.hi;
            range.hi = domain.lo;
            أرجع سچ;
        }
        إذا (!range.lo.isFourState()) intersectCrossRange(range, domain);
        إذا (range.wildcard && exprp->isSigned()) {
            ثابت صحيح sign = exprp->ويڪر() - 1;
            إذا (constp && !constp->isSigned() && constp->ويڪر() > exprp->ويڪر()) {
                // Unsigned equality preserves every target bit pattern if discarded bits are zero.
                لكل (صحيح ٻٽو_انگ_2 = exprp->ويڪر(); ٻٽو_انگ_2 < constp->ويڪر(); ++ٻٽو_انگ_2) {
                    إذا (constp->انگ().ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {
                        range.lo = domain.hi;
                        range.hi = domain.lo;
                        أرجع سچ;
                    }
                }
                لكل (صحيح ٻٽو_انگ_2 = exprp->ويڪر(); ٻٽو_انگ_2 < range.نمونو_2.ويڪر(); ++ٻٽو_انگ_2) {
                    إذا (range.نمونو_2.ٻٽو_انگ_Is_XZ(sign))
                        range.نمونو_2.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 'x');
                    وإلا
                        range.نمونو_2.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, range.نمونو_2.ٻٽو_انگ_Is1(sign));
                }
                أرجع سچ;
            }
            // Discarded high bits constrain the target sign, rather than becoming don't-cares.
            لكل (صحيح ٻٽو_انگ_2 = exprp->ويڪر(); ٻٽو_انگ_2 < range.نمونو_2.ويڪر(); ++ٻٽو_انگ_2) {
                إذا (range.نمونو_2.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) استمر;
                ثابت منطقي قدر = range.نمونو_2.ٻٽو_انگ_Is1(ٻٽو_انگ_2);
                إذا (!range.نمونو_2.ٻٽو_انگ_Is_XZ(sign) && range.نمونو_2.ٻٽو_انگ_Is1(sign) != قدر) {
                    range.lo = domain.hi;
                    range.hi = domain.lo;
                    اكسر;
                }
                range.نمونو_2.سيٽ_ٻٽو_انگ(sign, قدر);
            }
        }
        أرجع سچ;
    }

    تعداد CrossRangeState : uint8_t {
        CROSS_INSIDE_BOUNDS = 0,  // Prefix is strictly inside the interval
        CROSS_AT_هيٺاهون = 1,
        CROSS_AT_مٿانهون = 2,
        CROSS_AT_BOUNDS = CROSS_AT_هيٺاهون | CROSS_AT_مٿانهون,
        CROSS_NO_ميلاپ = 4  // Prefix cannot match the interval/pattern
    };

    ساكن CrossRangeState cross_Range_قدم(ثابت Cross_قدر_Range& range, CrossRangeState state,
                                          صحيح ٻٽو_انگ_2, صحيح قدر) {
        إذا (state == CROSS_NO_ميلاپ) أرجع CROSS_NO_ميلاپ;
        // Flipping the sign bit makes signed order lexicographic.
        ثابت منطقي sign = ٻٽو_انگ_2 == range.نمونو_2.ويڪر() - 1;
        إذا (!range.نمونو_2.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2) && قدر != (range.نمونو_2.ٻٽو_انگ_Is1(ٻٽو_انگ_2) ^ sign)) {
            أرجع CROSS_NO_ميلاپ;
        }
        ثابت صحيح هيٺيون = range.lo.ٻٽو_انگ_Is1(ٻٽو_انگ_2) ^ sign;
        ثابت صحيح مٿاهون = range.hi.ٻٽو_انگ_Is1(ٻٽو_انگ_2) ^ sign;
        إذا (((state & CROSS_AT_هيٺاهون) && قدر < هيٺيون)
            || ((state & CROSS_AT_مٿانهون) && قدر > مٿاهون)) {
            أرجع CROSS_NO_ميلاپ;
        }
        أرجع static_cast<CrossRangeState>(
            ((state & CROSS_AT_هيٺاهون) && قدر == هيٺيون ? CROSS_AT_هيٺاهون : CROSS_INSIDE_BOUNDS)
            | ((state & CROSS_AT_مٿانهون) && قدر == مٿاهون ? CROSS_AT_مٿانهون : CROSS_INSIDE_BOUNDS));
    }

    ساكن منطقي crossWildcardIntersects(ثابت Cross_قدر_Range& range) {
        بنانشان حالتون = 1U << CROSS_AT_BOUNDS;
        لكل (صحيح ٻٽو_انگ_2 = range.نمونو_2.ويڪر() - 1; ٻٽو_انگ_2 >= 0 && حالتون; --ٻٽو_انگ_2) {
            بنانشان ايندڙ = 0;
            لكل (ثابت CrossRangeState state :
                 {CROSS_INSIDE_BOUNDS, CROSS_AT_هيٺاهون, CROSS_AT_مٿانهون, CROSS_AT_BOUNDS}) {
                إذا (!(حالتون & (1U << state))) استمر;
                لكل (صحيح قدر = 0; قدر < 2; ++قدر) {
                    ثابت CrossRangeState equal = cross_Range_قدم(range, state, ٻٽو_انگ_2, قدر);
                    إذا (equal != CROSS_NO_ميلاپ) ايندڙ |= 1U << equal;
                }
            }
            حالتون = ايندڙ;
        }
        أرجع حالتون != 0;
    }

    // The least value at least 'from' with the non-x bits of 'pattern', in unsigned order; false
    // if none.  A bit scan, which does not enumerate the pattern's x bits.
    ساكن منطقي ايندڙ_نمونو_قدر(ثابت V3_عدد& نمونو_2, ثابت V3_عدد& from, V3_عدد& نتيجو) {
        نتيجو.op_سونپيو(from);
        صحيح حاصل = -1;  // Lowest x bit above the bit scanned where 'from' has a 0
        لكل (صحيح ٻٽو_انگ_2 = from.ويڪر() - 1; ٻٽو_انگ_2 >= 0; --ٻٽو_انگ_2) {
            إذا (نمونو_2.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2)) {
                إذا (!from.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) حاصل = ٻٽو_انگ_2;
                استمر;
            }
            إذا (نمونو_2.ٻٽو_انگ_Is1(ٻٽو_انگ_2) == from.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) استمر;
            إذا (from.ٻٽو_انگ_Is1(ٻٽو_انگ_2)) {  // Only a greater prefix, an x bit above raised, can match
                إذا (حاصل < 0) أرجع خطأ;
                ٻٽو_انگ_2 = حاصل;
            }
            // Then the least such value: the pattern's bits, with x bits of zero
            نتيجو.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
            طالما (--ٻٽو_انگ_2 >= 0) نتيجو.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, نمونو_2.ٻٽو_انگ_Is1(ٻٽو_انگ_2));
            أرجع سچ;
        }
        أرجع سچ;
    }

    // Append to 'runs' the maximal runs of consecutive values of 'range' that its pattern
    // matches, in value order, until there are more than 'limit'.  A match continues through
    // the pattern's trailing x bits only, which the next value's carry leaves.
    ساكن فراغ crossRangeRuns(ثابت Cross_قدر_Range& range, سائيز_t حد_بندي,
                               std::ويڪٽر<std::pair<V3_عدد, V3_عدد>>& runs) {
        // Values order signed, which is the unsigned order of values with the sign bit flipped
        ثابت صحيح sign = range.lo.ويڪر() - 1;
        ثابت خودڪار flipped = [sign](V3_عدد قدر) {
            إذا (!قدر.ٻٽو_انگ_Is_XZ(sign)) قدر.سيٽ_ٻٽو_انگ(sign, !قدر.ٻٽو_انگ_Is1(sign));
            أرجع قدر;
        };
        ثابت V3_عدد نمونو_2 = flipped(range.نمونو_2);
        ثابت V3_عدد hi = flipped(range.hi);
        ثابت V3_عدد one{&hi, hi.ويڪر(), 1};
        V3_عدد trailing{&hi, hi.ويڪر()};
        لكل (صحيح ٻٽو_انگ_2 = 0; ٻٽو_انگ_2 <= sign && نمونو_2.ٻٽو_انگ_Is_XZ(ٻٽو_انگ_2); ++ٻٽو_انگ_2) trailing.سيٽ_ٻٽو_انگ(ٻٽو_انگ_2, 1);
        V3_عدد from = flipped(range.lo);
        V3_عدد پهريون{&hi, hi.ويڪر()};
        V3_عدد آخري{&hi, hi.ويڪر()};
        V3_عدد less{&hi};
        طالما (runs.سائيز() <= حد_بندي && ايندڙ_نمونو_قدر(نمونو_2, from, پهريون)
               && less.opLt(hi, پهريون).isEqZero()) {
            آخري.opOr(پهريون, trailing);
            إذا (!less.opLt(hi, آخري).isEqZero()) آخري = hi;
            runs.emplace_پٺ(flipped(پهريون), flipped(آخري));
            إذا (آخري.isCaseEq(hi)) اكسر;
            from.op_جوڙيو(آخري, one);
        }
    }

    // True if no coverpoint value participates in a resolved value or range.  A value with x
    // or z bits participates only as a wildcard pattern (IEEE 1800-2023 19.5.7).
    ساكن منطقي cross_Range_خالي_جايون(ثابت Cross_قدر_Range& range) {
        إذا (range.lo.isFourState()) أرجع سچ;
        أرجع cross_قدر_Less(range.hi, range.lo)
               || (range.wildcard && !crossWildcardIntersects(range));
    }

    // Comparison width that holds both the coverpoint's and a bin or intersect value's range.
    ساكن صحيح resolve_ويڪر(Ast_ڳنڍ* nodep, ثابت Ast_ڳنڍ_اظهار* exprp) {
        أرجع std::max(exprp->ويڪر(), cross_Range_ويڪر(nodep)) + 1;
    }

    // Resolve a bin or intersect value to coverpoint values (IEEE 1800-2023 19.5.7), in the
    // width 'range' was created with.  False if the value is not a constant integral or real.
    ساكن منطقي resolve_قدر(Ast_ڳنڍ* nodep, Ast_ڳنڍ_اظهار* exprp, منطقي bin_قدر, منطقي wildcard,
                             Cross_قدر_Range& range) {
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(nodep, exprp->ويڪر(), exprp->isSigned(), range.lo.ويڪر());
        أرجع cross_قدر_Range(nodep, exprp, bin_قدر, wildcard, domain, range);
    }

    ساكن منطقي crossRangesIntersect(ثابت Cross_قدر_Range& bin, ثابت Cross_قدر_Range& filter) {
        // Values with x or z bits do not participate, even in an identical filter.
        إذا (bin.lo.isFourState() || filter.lo.isFourState()) أرجع خطأ;
        Cross_قدر_Range ميلاپ = bin;
        intersectCrossRange(ميلاپ, filter);
        أرجع !cross_قدر_Less(ميلاپ.hi, ميلاپ.lo)
               && (!bin.wildcard || crossWildcardIntersects(ميلاپ));
    }

    ساكن فراغ unsupportedCrossRange(AstCoverBinsof* selectp, منطقي& valid) {
        selectp->v3warn(COVERIGN, "Unsupported: non-constant or non-integral 'intersect' value, "
                                  "or four-state range bound.");
        valid = خطأ;
    }

    ساكن منطقي cross_قدر_Matches_Filters(AstCoverBinsof* selectp, Ast_ڳنڍ* valuep,
                                         Ast_ڳنڍ_اظهار* exprp, ثابت AstCoverBin* binp,
                                         ثابت Cross_قدر_Range& domain,
                                         ثابت std::ويڪٽر<Cross_قدر_Range>& filters,
                                         منطقي& valid) {
        Cross_قدر_Range range{valuep, domain.lo.ويڪر()};
        إذا (!cross_قدر_Range(valuep, exprp, سچ, binp->isWildcard(), domain, range)) {
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        لكل (ثابت Cross_قدر_Range& filter : filters) {
            إذا (crossRangesIntersect(range, filter)) أرجع سچ;
        }
        أرجع خطأ;
    }

    std::ويڪٽر<منطقي> selectCoverpointBins(AstCoverBinsof* selectp, ثابت CoverpointBins& bins,
                                           uint32_t پهريون, uint32_t ڳڻپ, منطقي& valid) {
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            unsupportedCrossRange(selectp, valid);
            أرجع {};
        }
        std::ويڪٽر<منطقي> selected(bins.ڪل, خطأ);
        std::ويڪٽر<std::ويڪٽر<Ast_ڳنڍ*>> قدرون;
        صحيح ويڪر = bins.exprp->ويڪر();
        لكل (Ast_ڳنڍ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            ويڪر = std::max(ويڪر, cross_Range_ويڪر(rangep));
        }
        إذا (selectp->rangesp()) {
            قدرون.reserve(ڳڻپ);
            لكل (uint32_t i = پهريون; i < پهريون + ڳڻپ; ++i) {
                // A run's values are in the coverpoint type, whose width 'width' covers
                قدرون.ڌڪو_پٺ(bins.قدرون[i].runp ? std::ويڪٽر<Ast_ڳنڍ*>{}
                                                     : cross_Bin_قدرون(bins.قدرون[i]));
                لكل (Ast_ڳنڍ* ثابت valuep : قدرون.پٺ()) {
                    ويڪر = std::max(ويڪر, cross_Range_ويڪر(valuep));
                }
            }
        }
        // One extra bit preserves both unsigned maxima and negative signed bounds.
        ++ويڪر;
        ثابت Cross_قدر_Range domain
            = cross_قدر_Domain(selectp, bins.exprp->ويڪر(), bins.exprp->isSigned(), ويڪر);
        std::ويڪٽر<Cross_قدر_Range> filters;
        لكل (Ast_ڳنڍ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range filter{rangep, ويڪر};
            إذا (!cross_قدر_Range(rangep, bins.exprp, خطأ, خطأ, domain, filter)) {
                unsupportedCrossRange(selectp, valid);
                أرجع {};
            }
            filters.ڌڪو_پٺ(std::move(filter));
        }
        لكل (uint32_t i = پهريون; i < پهريون + ڳڻپ; ++i) {
            إذا (!selectp->rangesp()) {
                selected[i] = سچ;
                استمر;
            }
            إذا (ثابت Bin_لاڳيتو* ثابت runp = bins.قدرون[i].runp) {
                ثابت Cross_قدر_Range range
                    = لاڳيتو_Bin_Range(selectp, *runp, bins.قدرون[i].element, ويڪر);
                selected[i] = std::any_جو(filters.begin(), filters.پڇاڙي(),
                                          [&](ثابت Cross_قدر_Range& filter) {
                                              أرجع crossRangesIntersect(range, filter);
                                          });
                استمر;
            }
            لكل (Ast_ڳنڍ* ثابت valuep : قدرون[i - پهريون]) {
                selected[i] = cross_قدر_Matches_Filters(
                    selectp, valuep, bins.exprp, bins.قدرون[i].binp, domain, filters, valid);
                إذا (!valid) أرجع {};
                إذا (selected[i]) اكسر;
            }
        }
        إذا (selectp->isNegated()) {
            لكل (uint32_t i = 0; i < bins.ڪل; ++i) selected[i] = !selected[i];
        }
        أرجع selected;
    }

    // Constructor-time value metadata of one coverpoint, as C++ list entries
    بنية قدر_Lists final {
        std::ويڪٽر<std::string> m_ranges;  // Bin, then low and high words
        std::ويڪٽر<std::string> m_runs;  // First bin, count, then low, span, and high words
        std::ويڪٽر<std::string> m_patterns;  // Bin, then value, mask, low, and high words
        std::ويڪٽر<std::string> m_transitions;  // Transition bin
    };

    // Append a value's words, in the coverpoint's width, to a C++ list entry.
    ساكن فراغ append_لفظ(std::string& متن, ثابت V3_عدد& قدر, ثابت Ast_ڳنڍ_اظهار* exprp) {
        V3_عدد narrowed{&قدر, exprp->ويڪر()};
        narrowed.op_سونپيو(قدر);
        لكل (صحيح لفظ = 0; لفظ < exprp->ويڪر_لفظ(); ++لفظ) {
            متن += ", " + cvt_منزل_Str(narrowed.edata_لفظ(لفظ)) + "U";
        }
    }

    فراغ collect_قدر_Metadata(قدر_Lists& lists, Ast_ڳنڍ_اظهار* exprp, AstCoverBin* binp,
                              uint32_t اشاريو, Ast_ڳنڍ_اظهار* valuep) {
        ثابت std::string bin = cvt_منزل_Str(اشاريو) + "U";
        إذا (binp->transp()) lists.m_transitions.ڌڪو_پٺ(bin);
        لكل (Ast_ڳنڍ* ثابت sourcep : cross_Bin_قدرون({binp, valuep})) {
            Cross_قدر_Range range{sourcep, resolve_ويڪر(sourcep, exprp)};
            إذا (!resolve_قدر(sourcep, exprp, سچ, binp->isWildcard(), range)) {
                // Sampling already resolved every state bin value, so only transitions remain.
                sourcep->v3warn(E_UNSUPPORTED, "Unsupported: non-integral value in a transition "
                                               "bin of a coverpoint with exclusions.");
                استمر;
            }
            إذا (cross_Range_خالي_جايون(range)) استمر;
            std::string داخلا_2 = bin;
            إذا (range.wildcard) {
                V3_عدد قدر{sourcep, exprp->ويڪر()};
                V3_عدد نقاب{sourcep, exprp->ويڪر()};
                نقاب.op_بٽ_Non_XZ(range.نمونو_2);
                قدر.op_بٽ_One(range.نمونو_2);
                append_لفظ(داخلا_2, قدر, exprp);
                append_لفظ(داخلا_2, نقاب, exprp);
            }
            append_لفظ(داخلا_2, range.lo, exprp);
            append_لفظ(داخلا_2, range.hi, exprp);
            (range.wildcard ? lists.m_patterns : lists.m_ranges).ڌڪو_پٺ(داخلا_2);
        }
    }

    // Describe a run with one entry, from which the runtime computes the values of its bins.
    ساكن فراغ collect_لاڳيتو_Metadata(قدر_Lists& lists, Ast_ڳنڍ_اظهار* exprp, ثابت Bin_لاڳيتو& لاڳيتو) {
        V3_عدد وسعت{exprp, لاڳيتو.m_stride.ويڪر(), 0};
        وسعت.opSub(لاڳيتو.m_stride, V3_عدد{exprp, لاڳيتو.m_stride.ويڪر(), 1});
        std::string داخلا_2 = cvt_منزل_Str(لاڳيتو.m_declared) + "U, " + cvt_منزل_Str(لاڳيتو.m_ڳڻپ) + "U";
        append_لفظ(داخلا_2, لاڳيتو.m_lo, exprp);
        append_لفظ(داخلا_2, وسعت, exprp);
        append_لفظ(داخلا_2, لاڳيتو.m_hi, exprp);
        lists.m_runs.ڌڪو_پٺ(داخلا_2);
    }

    // Emit one batched metadata list, bounding the size of each call's temporary list.
    فراغ emit_قدر_List(File_سٽ* fl, AstVar* cpVarp, VCMethod طريقو,
                       ثابت std::ويڪٽر<std::string>& entries) {
        لكل (سائيز_t پهريون = 0; پهريون < entries.سائيز(); پهريون += قدر_LIST_ENTRIES) {
            ثابت سائيز_t پڇاڙي = std::min(entries.سائيز(), پهريون + قدر_LIST_ENTRIES);
            std::string متن = "{" + entries[پهريون];
            لكل (سائيز_t i = پهريون + 1; i < پڇاڙي; ++i) متن += ", " + entries[i];
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cpVarp, طريقو, {ctext(fl, متن + "}")})->makeStmt());
        }
    }

    ساكن منطقي checkCrossRef(ثابت AstCoverCrossRef* refp, ثابت AstCoverCross* crossp) {
        إذا (refp->name() == crossp->name()) أرجع سچ;
        refp->v3error("Cross selection "
                      << refp->prettyNameQ() << " may only name its enclosing cross "
                      << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1.2).");
        أرجع خطأ;
    }

    ساكن منطقي checkCrossBinName(ثابت AstCoverCrossBin* binp, std::سيٽ<std::string>& نالا) {
        إذا (نالا.emplace(binp->name()).ٻيو) أرجع سچ;
        binp->v3error("Duplicate cross bin " << binp->prettyNameQ()
                                             << " (IEEE 1800-2023 19.6.1).");
        أرجع خطأ;
    }

    // The span of the implicit automatic bin reported as 'name' ('auto_<i>', see
    // createImplicitAutoBins), found without naming each of its bins
    ساكن منطقي implicit_Auto_Bin_وسعت(ثابت CoverpointBins& bins, ثابت std::string& name,
                                    Bin_وسعت& وسعت) {
        ثابت std::string اڳياڙي = "auto_";
        إذا (!VString::startsWith(name, اڳياڙي)) أرجع خطأ;
        ثابت std::string digits = name.substr(اڳياڙي.سائيز());
        ثابت بنانشان ڊگهو اشاريو = std::strtoul(digits.c_str(), خالي_اشارو, 10);
        // Only the reported spelling names the bin, not e.g. 'auto_01' or 'auto_x'
        إذا (اشاريو >= bins.implicitAuto.ڳڻپ || digits != cvt_منزل_Str(اشاريو)) أرجع خطأ;
        ثابت uint32_t وٿي = static_cast<uint32_t>(اشاريو);
        وسعت = Bin_وسعت{bins.implicitAuto.پهريون + وٿي, 1, bins.implicitAuto.declared + وٿي};
        أرجع سچ;
    }

    Cross_Binsof_هدف
    resolve_Binsof_هدف(ثابت AstCoverBinsof* selectp, ثابت AstCoverCross* crossp,
                        ثابت std::ويڪٽر<AstVar*>& cpVars,
                        ثابت std::map<std::string, uint32_t>& dimensions) ثابت {
        ثابت خودڪار dim = dimensions.ڳوليو(selectp->pointp()->name());
        إذا (dim == dimensions.پڇاڙي()) {
            selectp->v3error("binsof coverpoint "
                             << selectp->pointp()->prettyNameQ() << " is not an item of cross "
                             << crossp->prettyNameQ() << " (IEEE 1800-2023 19.6.1).");
            أرجع {};
        }
        ثابت CoverpointBins& bins = m_cpBins.at(cpVars[dim->ٻيو]);
        Cross_Binsof_هدف هدف{&bins, dim->ٻيو, 0, bins.ڪل};
        إذا (!selectp->name().خالي_جايون()) {
            ثابت خودڪار bin = bins.spans.ڳوليو(selectp->name());
            Bin_وسعت وسعت{0, 0, 0};
            إذا (bin != bins.spans.پڇاڙي()) {
                وسعت = bin->ٻيو;
            } وإلا إذا (!implicit_Auto_Bin_وسعت(bins, selectp->name(), وسعت)) {
                selectp->v3error("Cannot find bin " << selectp->prettyNameQ() << " in coverpoint "
                                                    << selectp->pointp()->prettyNameQ()
                                                    << " (IEEE 1800-2023 19.6.1).");
                أرجع {};
            }
            هدف.m_پهريون = وسعت.پهريون;
            هدف.m_ڳڻپ = وسعت.ڳڻپ;
            هدف.m_declared_پهريون = وسعت.declared;
            هدف.m_declared_پڇاڙي = وسعت.declared + وسعت.ڳڻپ;
            هدف.m_sized = وسعت.sized;
        }
        أرجع هدف;
    }

    // The variables a 'with' filter reads, each by its first reference
    استعمال CrossFilterReads = std::map<ثابت AstVar*, AstVarRef*>;

    // Whether a 'with' filter has a count of value tuples, of 'matches' other than '$'
    ساكن منطقي counts_قدر_Tuples(ثابت AstCoverCrossWith* withp) {
        أرجع withp->countp() && !VN_IS(withp->countp(), Unbounded);
    }

    // Whether the runtime knows the values of the cross items a 'with' filter reads, or to count
    // value tuples, of all items, of integral coverpoints; else warns
    ساكن منطقي crossFilterSupported(ثابت AstCoverCrossWith* withp,
                                     ثابت CrossFilterReads& reads) {
        لكل (ثابت AstVar* varp = withp->varsp(); varp; varp = VN_AS(varp->nextp(), Var)) {
            إذا (varp->dtypep()->skipRefp()->isIntegralOrPacked()) استمر;
            ثابت خودڪار it = reads.ڳوليو(varp);
            إذا (it != reads.پڇاڙي()) {
                it->ٻيو->v3warn(COVERIGN, "Unsupported: cross bin 'with' filter reading the "
                                             "value of non-integral coverpoint "
                                                 << it->ٻيو->prettyNameQ());
                أرجع خطأ;
            }
            إذا (counts_قدر_Tuples(withp)) {
                withp->countp()->v3warn(COVERIGN, "Unsupported: cross bin 'matches' count of "
                                                  "value tuples of non-integral coverpoint "
                                                      << varp->prettyNameQ());
                أرجع خطأ;
            }
        }
        أرجع سچ;
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
                             uint32_t شيون) {
        File_سٽ* ثابت fl = withp->fileline();
        ثابت string اڳياڙي
            = m_sized_نالا.get(sanitizeGeneratedName(crossp->name() + "__" + binp->name()));
        ثابت منطقي counting = counts_قدر_Tuples(withp);
        // The count, 0 for all
        ثابت std::pair<Ast_ڳنڍ_اظهار*, Ast_ڳنڍ_اظهار*> ڳڻپ
            = counting ? new_ڳڻپ_دليلون(fl, اڳياڙي + "_count", withp->countp())
                       : new_ڳڻپ_دليلون(fl, withp->countp() ? 0 : 1);
        AstLoop* ثابت loopp = جديد AstLoop{fl};
        AstCMethodHard* ثابت nextp = شي_Call(fl, cxp, VCMethod::COVERGROUP_WITH_ايندڙ);
        nextp->dtype_سيٽ_ٻٽو_انگ();
        loopp->جوڙيو_Stmtsp(جديد AstLoopTest{fl, loopp, nextp});
        string dims;
        لكل (uint32_t dim = 0; counting && dim < شيون; ++dim) {
            dims += (dims.خالي_جايون() ? "" : ", ") + cvt_منزل_Str(dim) + "U";
        }
        طالما (AstVar* ثابت varp = withp->varsp()) {
            varp->unlink_Fr_پٺ();
            إذا (!reads.ڳڻپ(varp)) {
                VL_DO_DANGLING(ڌڪو_Deletep(varp), varp);
                استمر;
            }
            ثابت خودڪار dimIt = dimensions.ڳوليو(varp->name());
            UASSERT_OBJ(dimIt != dimensions.پڇاڙي(), varp, "Cross 'with' candidate not of an item");
            ثابت uint32_t dim = dimIt->ٻيو;
            إذا (!counting) dims += (dims.خالي_جايون() ? "" : ", ") + cvt_منزل_Str(dim) + "U";
            varp->name(اڳياڙي + "_" + sanitizeGeneratedName(varp->name()));
            m_constructorp->جوڙيو_Stmtsp(varp);
            إذا (varp->isWide()) {
                loopp->جوڙيو_Stmtsp(شي_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر_W,
                                          {cnum(fl, dim), جديد AstVarRef{fl, varp, VAccess::WRITE}})
                                     ->makeStmt());
            } وإلا {
                AstCMethodHard* ثابت valuep
                    = شي_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_قدر, {cnum(fl, dim)});
                valuep->dtype_سيٽ_UInt64();
                Ast_ڳنڍ_DType* ثابت dtypep = varp->ڳوليو_Logic_DType(
                    varp->ويڪر(), varp->ويڪر(),
                    varp->isSigned() ? VSigning::SIGNED : VSigning::UNSIGNED);
                loopp->جوڙيو_Stmtsp(جديد Ast_سونپيو{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                               resize_قدر(valuep, dtypep)});
            }
        }
        loopp->جوڙيو_Stmtsp(شي_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_TEST,
                                  {withp->filterp()->unlink_Fr_پٺ()})
                             ->makeStmt());
        ثابت منطقي prot = v3Global.opt.protectIds();
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_BEGIN,
                     {ctext(fl, "{" + dims + "}"), ڳڻپ.پهريون, ڳڻپ.ٻيو,
                      ctext(fl, quoted(VIdProtect::protect_لفظ_If(binp->name(), prot))),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno()))})
                ->makeStmt());
        m_constructorp->جوڙيو_Stmtsp(loopp);
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cxp, VCMethod::COVERGROUP_CROSS_WITH_پڇاڙي)->makeStmt());
    }

    منطقي generateRuntimeSelection(Ast_ڳنڍ* nodep, ثابت AstCoverCrossBin* binp,
                                  AstCoverCross* crossp, AstVar* cxp,
                                  ثابت std::ويڪٽر<AstVar*>& cpVars,
                                  ثابت std::map<string, uint32_t>& dimensions) {
        File_سٽ* ثابت fl = nodep->fileline();
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, crossp)) أرجع خطأ;
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_ALL)->makeStmt());
            أرجع سچ;
        }
        إذا (ثابت AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            إذا (!generateRuntimeSelection(opp->lhsp(), binp, crossp, cxp, cpVars, dimensions)
                || !generateRuntimeSelection(opp->rhsp(), binp, crossp, cxp, cpVars, dimensions)) {
                أرجع خطأ;
            }
            m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cxp,
                                               opp->isOr() ? VCMethod::COVERGROUP_SELECT_OR
                                                           : VCMethod::COVERGROUP_SELECT_AND)
                                          ->makeStmt());
            أرجع سچ;
        }
        إذا (ثابت AstCoverCrossWith* ثابت withp = VN_CAST(nodep, CoverCrossWith)) {
            CrossFilterReads reads;
            withp->filterp()->foreach([&](AstVarRef* refp) { reads.emplace(refp->varp(), refp); });
            // The count of 'matches' is a positive integer, or '$' (IEEE 1800-2023 19.6.1.2)
            إذا ((counts_قدر_Tuples(withp)
                 && !check_ڳڻپ(withp->countp(), "Cross bin 'matches' count", "19.6.1.2"))
                || !crossFilterSupported(withp, reads)
                || !generateRuntimeSelection(withp->selectp(), binp, crossp, cxp, cpVars,
                                             dimensions)) {
                أرجع خطأ;
            }
            generateCrossFilter(withp, reads, binp, crossp, cxp, dimensions,
                                static_cast<uint32_t>(cpVars.سائيز()));
            أرجع سچ;
        }
        AstCoverBinsof* ثابت selectp = VN_AS(nodep, CoverBinsof);
        ثابت Cross_Binsof_هدف هدف = resolve_Binsof_هدف(selectp, crossp, cpVars, dimensions);
        إذا (!هدف.m_binsp) أرجع خطأ;
        ثابت CoverpointBins& bins = *هدف.m_binsp;
        إذا (selectp->rangesp() && !bins.exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            منطقي valid = سچ;
            unsupportedCrossRange(selectp, valid);
            أرجع خطأ;
        }
        Ast_ڳنڍ_اظهار* firstp;
        Ast_ڳنڍ_اظهار* endp;
        إذا (هدف.m_sized < 0) {
            firstp = cnum(fl, هدف.m_declared_پهريون);
            endp = cnum(fl, هدف.m_declared_پڇاڙي);
        } وإلا {  // A sized array, whose bins the coverpoint's construction placed
            AstVar* ثابت cpVarp = cpVars[هدف.m_dimension];
            ثابت uint32_t sized = static_cast<uint32_t>(هدف.m_sized);
            firstp = شي_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_پهريون, {cnum(fl, sized)});
            endp = شي_Call(fl, cpVarp, VCMethod::COVERGROUP_SIZED_پڇاڙي, {cnum(fl, sized)});
            firstp->dtype_سيٽ_UInt32();
            endp->dtype_سيٽ_UInt32();
        }
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM,
                     {cnum(fl, هدف.m_dimension), firstp, endp, cnum(fl, selectp->isNegated()),
                      cnum(fl, selectp->rangesp() != خالي_اشارو)})
                ->makeStmt());
        لكل (Ast_ڳنڍ* rangep = selectp->rangesp(); rangep; rangep = rangep->nextp()) {
            Cross_قدر_Range range{rangep, resolve_ويڪر(rangep, bins.exprp)};
            إذا (!resolve_قدر(rangep, bins.exprp, خطأ, خطأ, range)) {
                منطقي valid = سچ;
                unsupportedCrossRange(selectp, valid);
                أرجع خطأ;
            }
            إذا (cross_Range_خالي_جايون(range)) استمر;
            m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cxp,
                                               bins.exprp->isWide()
                                                   ? VCMethod::COVERGROUP_SELECT_RANGE_W
                                                   : VCMethod::COVERGROUP_SELECT_RANGE,
                                               {new_قدر_Const(fl, range.lo, bins.exprp),
                                                new_قدر_Const(fl, range.hi, bins.exprp)})
                                          ->makeStmt());
        }
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_DIM_پڇاڙي)->makeStmt());
        أرجع سچ;
    }

    std::ويڪٽر<AstCoverCrossBin*>
    generateRuntimeCrossBins(AstCoverCross* crossp, AstVar* cxp,
                             ثابت std::ويڪٽر<AstVar*>& cpVars,
                             ثابت std::map<string, uint32_t>& dimensions) {
        std::ويڪٽر<AstCoverCrossBin*> bins;
        std::سيٽ<string> نالا;
        لكل (Ast_ڳنڍ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, نالا)) استمر;
            File_سٽ* ثابت fl = binp->fileline();
            // Generate the selection apart, so that an unsupported one leaves no code, which
            // could leave its state for the next bin
            AstFunc* ثابت selectionp = جديد AstFunc{fl, "", خالي_اشارو, خالي_اشارو};
            منطقي supported;
            {
                VL_RESTORER(m_constructorp);
                m_constructorp = selectionp;
                supported = generateRuntimeSelection(binp->selectp(), binp, crossp, cxp, cpVars,
                                                     dimensions);
            }
            // A supported selection has code
            إذا (supported) m_constructorp->جوڙيو_Stmtsp(selectionp->stmtsp()->unlink_Fr_پٺ_With_ايندڙ());
            VL_DO_DANGLING(ڌڪو_Deletep(selectionp), selectionp);
            إذا (!supported) استمر;
            ثابت منطقي protect = v3Global.opt.protectIds();
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cxp, VCMethod::COVERGROUP_SELECT_BIN,
                         {ctext(fl, binp->binsType().bin_سيٽ_Enum()),
                          ctext(fl, quoted(VIdProtect::protect_لفظ_If(binp->name(), protect))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), protect))),
                          cnum(fl, fl->lineno()), cnum(fl, fl->پهريون_ڪالم()),
                          cnum(fl, static_cast<uint32_t>(bins.سائيز()))})
                    ->makeStmt());
            bins.ڌڪو_پٺ(binp);
        }
        m_constructorp->جوڙيو_Stmtsp(
            شي_Call(crossp->fileline(), cxp, VCMethod::COVERGROUP_FINALIZE_BINS)->makeStmt());
        أرجع bins;
    }

    ساكن فراغ سيٽ_Cross_Selection_Range(CrossSelection& selection, uint64_t پهريون, uint64_t پڇاڙي) {
        طالما (پهريون < پڇاڙي) {
            ثابت بنانشان ٻٽو_انگ_2 = پهريون % 64;
            ثابت بنانشان بٽ = std::min<uint64_t>(64 - ٻٽو_انگ_2, پڇاڙي - پهريون);
            selection[VL_BITWORD_Q(پهريون)]
                |= (بٽ == 64 ? ~uint64_t{0} : (uint64_t{1} << بٽ) - 1) << ٻٽو_انگ_2;
            پهريون += بٽ;
        }
    }

    CrossSelection crossSelection(Ast_ڳنڍ* nodep, CrossSelectionContext& ctx) {
        إذا (ثابت AstCoverCrossRef* ثابت refp = VN_CAST(nodep, CoverCrossRef)) {
            إذا (!checkCrossRef(refp, ctx.crossp)) {
                ctx.valid = خطأ;
                أرجع {};
            }
            CrossSelection نتيجو(
                VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1), 0);
            سيٽ_Cross_Selection_Range(نتيجو, 0, ctx.tuples);
            أرجع نتيجو;
        }
        إذا (AstCoverCrossSelect* ثابت opp = VN_CAST(nodep, CoverCrossSelect)) {
            CrossSelection lhs = crossSelection(opp->lhsp(), ctx);
            ثابت CrossSelection rhs = crossSelection(opp->rhsp(), ctx);
            إذا (!ctx.valid) أرجع {};
            لكل (سائيز_t i = 0; i < lhs.سائيز(); ++i) {
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
        ثابت std::ويڪٽر<منطقي> selected
            = selectCoverpointBins(selectp, bins, هدف.m_پهريون, هدف.m_ڳڻپ, ctx.valid);
        إذا (!ctx.valid) أرجع {};
        CrossSelection نتيجو(VL_BITWORD_Q(static_cast<uint64_t>(ctx.tuples) + VL_QUADSIZE - 1),
                              0);
        ثابت uint64_t stride = ctx.strides[هدف.m_dimension];
        ثابت uint64_t period = stride * bins.ڪل;
        لكل (uint64_t بنياد = 0; بنياد < ctx.tuples; بنياد += period) {
            لكل (uint32_t i = 0; i < bins.ڪل;) {
                إذا (!selected[i]) {
                    ++i;
                    استمر;
                }
                ثابت uint32_t begin = i++;
                طالما (i < bins.ڪل && selected[i]) ++i;
                سيٽ_Cross_Selection_Range(نتيجو, بنياد + begin * stride, بنياد + i * stride);
            }
        }
        أرجع نتيجو;
    }

    // Size the Cartesian product of the declared Normal bins, with each dimension's flat-index
    // stride.  Live runtime bins only shrink it.  Warns and returns false if it is too large.
    منطقي crossShape(AstCoverCross* crossp, ثابت std::ويڪٽر<AstVar*>& cpVars,
                    std::ويڪٽر<uint32_t>& strides, uint32_t& tuples) ثابت {
        uint64_t حاصل_ضرب = std::any_جو(cpVars.begin(), cpVars.پڇاڙي(),
                                       [هذا](AstVar* varp) { أرجع !m_cpBins.at(varp).ڪل; })
                               ? 0
                               : 1;
        strides.resize(cpVars.سائيز());
        لكل (سائيز_t d = cpVars.سائيز(); d > 0; --d) {
            strides[d - 1] = static_cast<uint32_t>(حاصل_ضرب);
            حاصل_ضرب *= m_cpBins.at(cpVars[d - 1]).ڪل;
            إذا (حاصل_ضرب > UINT32_MAX) {
                crossp->v3warn(COVERIGN,
                               "Unsupported: cross coverage with more than 2^32-1 tuples.");
                أرجع خطأ;
            }
        }
        tuples = static_cast<uint32_t>(حاصل_ضرب);
        أرجع سچ;
    }

    CrossLayout resolveCrossLayout(AstCoverCross* crossp, ثابت std::ويڪٽر<AstVar*>& cpVars,
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
        std::سيٽ<std::string> نالا;
        لكل (Ast_ڳنڍ* itemp = crossp->binsp(); itemp; itemp = itemp->nextp()) {
            AstCoverCrossBin* ثابت binp = VN_AS(itemp, CoverCrossBin);
            إذا (!checkCrossBinName(binp, نالا)) استمر;
            ctx.valid = سچ;
            CrossSelection selection = crossSelection(binp->selectp(), ctx);
            إذا (!ctx.valid || std::all_جو(selection.begin(), selection.پڇاڙي(), [](uint64_t لفظ) {
                    أرجع لفظ == 0;
                })) {
                استمر;
            }
            إذا (occupied.خالي_جايون()) {
                occupied.resize(selection.سائيز(), 0);
                excluded.resize(selection.سائيز(), 0);
            }
            لكل (سائيز_t i = 0; i < selection.سائيز(); ++i) {
                occupied[i] |= selection[i];
                إذا (!binp->binsType().bin_Is_عام()) excluded[i] |= selection[i];
            }
            layout.bins.ڌڪو_پٺ({binp, std::move(selection)});
        }
        إذا (!layout.bins.خالي_جايون()) {
            // IEEE 1800-2023 19.6.2/19.6.3: exclusions also remove tuples from named
            // bins, independently of declaration order and sampling guards.
            لكل (ResolvedCrossBin& resolved : layout.bins) {
                لكل (سائيز_t i = 0; i < resolved.selection.سائيز(); ++i) {
                    إذا (resolved.binp->binsType().bin_Is_عام()) {
                        resolved.selection[i] &= ~excluded[i];
                    }
                    إذا (resolved.selection[i]) ++layout.bin_لفظ;
                }
            }
            layout.bins.erase(std::remove_if(layout.bins.begin(), layout.bins.پڇاڙي(),
                                             [](ثابت ResolvedCrossBin& resolved) {
                                                 أرجع std::all_جو(
                                                     resolved.selection.begin(),
                                                     resolved.selection.پڇاڙي(),
                                                     [](uint64_t لفظ) { أرجع لفظ == 0; });
                                             }),
                              layout.bins.پڇاڙي());
            layout.autoBins = layout.tuples;
            لكل (ثابت uint64_t لفظ : occupied) {
                layout.autoBins -= static_cast<uint32_t>(std::bitset<VL_QUADSIZE>{لفظ}.ڳڻپ());
            }
        }
        أرجع layout;
    }

    AstCoverCrossDType* crossDType(File_سٽ* fl, uint32_t dimensions, ثابت CrossLayout& layout,
                                   منطقي dynamic = خطأ) {
        ثابت uint32_t bins = static_cast<uint32_t>(layout.bins.سائيز());
        ثابت CrossShape shape{dimensions,      layout.tuples,   bins,
                               layout.autoBins, layout.bin_لفظ, dynamic};
        AstCoverCrossDType*& typep = m_cxDTypes[shape];
        إذا (!typep) {
            typep = جديد AstCoverCrossDType{
                fl, dimensions, layout.tuples, bins, layout.autoBins, layout.bin_لفظ, dynamic};
            v3Global.rootp()->typeTablep()->جوڙيو_Typesp(typep);
        }
        أرجع typep;
    }

    std::ويڪٽر<AstCoverCrossBin*> generateCrossBins(AstCoverCross* crossp, AstVar* cxVarp,
                                                     ثابت CrossLayout& layout) {
        std::ويڪٽر<AstCoverCrossBin*> bins;
        لكل (ثابت ResolvedCrossBin& resolved : layout.bins) {
            AstCoverCrossBin* ثابت binp = resolved.binp;
            ثابت CrossSelection& selection = resolved.selection;
            File_سٽ* ثابت fl = binp->fileline();
            ثابت منطقي prot = v3Global.opt.protectIds();
            std::string نقاب = "{";
            لكل (سائيز_t i = 0; i < selection.سائيز(); ++i) {
                إذا (i) نقاب += ", ";
                نقاب += std::منزل_string(selection[i]) + "ULL";
            }
            نقاب += "}";
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(fl, cxVarp, VCMethod::COVERGROUP_جوڙيو_BIN,
                         {ctext(fl, binp->binsType().bin_سيٽ_Enum()), ctext(fl, نقاب),
                          ctext(fl, quoted(VIdProtect::protect_لفظ_If(binp->name(), prot))),
                          ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                          cnum(fl, static_cast<uint32_t>(fl->lineno())),
                          cnum(fl, static_cast<uint32_t>(fl->پهريون_ڪالم()))})
                    ->makeStmt());
            bins.ڌڪو_پٺ(binp);
        }
        إذا (!bins.خالي_جايون()) {
            m_constructorp->جوڙيو_Stmtsp(
                شي_Call(crossp->fileline(), cxVarp, VCMethod::COVERGROUP_FINALIZE_BINS)
                    ->makeStmt());
        }
        أرجع bins;
    }

    // Route a cross through a VlCoverCross member: emit the member, its constructor init +
    // registration, and the sample() call.  The feeding coverpoints are already generated
    // (their hit lists drive the cross). Each explicit bin adds one configuration call.
    فراغ generateCross(AstCoverCross* crossp) {
        File_سٽ* ثابت fl = crossp->fileline();
        ثابت منطقي dynamic = m_runtimeCrosses.ڳڻپ(crossp);
        UINFO(4, "  Generating VlCoverCross member: " << crossp->name());

        إذا (Ast_ڳنڍ_اظهار* ثابت iffp = crossp->iffp()) {
            crossp->iffp(
                capture_Iff_منزل_عارضي(iffp, "__VcrossIff_" + sanitizeGeneratedName(crossp->name())));
        }

        // Resolve and unlink the coverpoint refs, in dimension order.  Every ref resolves to a
        // known coverpoint (a cross with an unresolvable item was dropped earlier).
        std::ويڪٽر<AstVar*> cpVars;
        std::map<std::string, uint32_t> dimensions;
        لكل (Ast_ڳنڍ* itemp = crossp->itemsp(); itemp;) {
            Ast_ڳنڍ* ثابت nextp = itemp->nextp();
            AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
            ثابت خودڪار it = m_cpVarMap.ڳوليو(refp->name());
            UASSERT_OBJ(it != m_cpVarMap.پڇاڙي(), crossp, "Cross references an unknown coverpoint");
            dimensions.emplace(refp->name(), static_cast<uint32_t>(cpVars.سائيز()));
            cpVars.ڌڪو_پٺ(it->ٻيو);
            VL_DO_DANGLING(ڌڪو_Deletep(refp->unlink_Fr_پٺ()), refp);
            itemp = nextp;
        }
        ثابت صحيح dims = static_cast<صحيح>(cpVars.سائيز());
        CrossLayout layout;
        إذا (dynamic) {
            // The runtime sizes the layout from live bins; only check the declared bound here.
            std::ويڪٽر<uint32_t> strides;
            uint32_t tuples = 0;
            layout.valid = crossShape(crossp, cpVars, strides, tuples);
        } وإلا {
            layout = resolveCrossLayout(crossp, cpVars, dimensions);
        }
        إذا (!layout.valid) أرجع;

        AstVar* ثابت cxVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcx_" + crossp->name(),
                         crossDType(fl, static_cast<uint32_t>(dims), layout, dynamic)};
        m_covergroupp->جوڙيو_Membersp(cxVarp);
        m_constructorp->جوڙيو_Stmtsp(make_شي_Create(fl, cxVarp,
                                                 dynamic ? VCMethod::COVERGROUP_جوڙيو_CROSS_DYN
                                                         : VCMethod::COVERGROUP_جوڙيو_CROSS));
        generate_شي_وزن(fl, cxVarp, crossp->optionsp());
        صحيح at_گهٽ_کان_گهٽ_قدر;
        صحيح autoBinMax;  // Not a cross option
        ڪڍو_شي_Options(crossp->optionsp(), at_گهٽ_کان_گهٽ_قدر, autoBinMax);

        // Constructor: init (after the coverpoints, which generate earlier) then registration.
        // Obfuscate the hierarchy/filename/page under --protect-ids as for coverpoints above.
        ثابت منطقي prot = v3Global.opt.protectIds();
        ثابت std::string hier
            = VIdProtect::protect_لفظ_If(m_covergroupName + "." + crossp->name(), prot);
        m_constructorp->جوڙيو_Stmtsp(makeCrossCpsCall(
            fl, cpVars,
            شي_Call(fl, cxVarp, VCMethod::COVERGROUP_INIT,
                     {ctext(fl, quoted(hier)), cnum(fl, static_cast<uint32_t>(at_گهٽ_کان_گهٽ_قدر)),
                      cnum(fl, static_cast<uint32_t>(dims)), ctext(fl, "__Vcx_cps"),
                      ctext(fl, quoted(VIdProtect::protectIf(fl->filename(), prot))),
                      cnum(fl, static_cast<uint32_t>(fl->lineno())),
                      cnum(fl, static_cast<uint32_t>(fl->پهريون_ڪالم()))})));
        ثابت std::ويڪٽر<AstCoverCrossBin*> bins
            = dynamic ? generateRuntimeCrossBins(crossp, cxVarp, cpVars, dimensions)
                      : generateCrossBins(crossp, cxVarp, layout);
        إذا (v3Global.opt.coverage()) {
            ثابت std::string صفحو
                = VIdProtect::protectIf("v_covergroup/" + m_covergroupName, prot);
            m_constructorp->جوڙيو_Stmtsp(شي_Call(fl, cxVarp, VCMethod::COVERGROUP_رجسٽر_BINS,
                                               {ctext(fl, "vlSymsp->_vm_contextp__->coveragep()"),
                                                ctext(fl, quoted(صفحو)),
                                                cnum(fl, شي_Database_وزن(crossp->optionsp())),
                                                cnum(fl, m_cg_Type_وزن)})
                                          ->makeStmt());
        }

        // sample(): after all coverpoints have sampled (cross loop runs after coverpoint loop).
        UASSERT_OBJ(m_sampleFuncp, crossp, "sample() CFunc not set for cross");
        // The cross remembers its feeding coverpoints, so sample() needs no cps array;
        // per-bin iff guards still need a temporary array, hence the block form.
        ثابت منطقي hasIffs
            = std::any_جو(bins.begin(), bins.پڇاڙي(),
                          [](ثابت AstCoverCrossBin* binp) { أرجع binp->iffp() != خالي_اشارو; });
        Ast_ڳنڍ_Stmt* ثابت samplep
            = !hasIffs ? static_cast<Ast_ڳنڍ_Stmt*>(
                             شي_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE)->makeStmt())
                       : static_cast<Ast_ڳنڍ_Stmt*>(makeCrossIffsCall(
                             fl, bins,
                             شي_Call(fl, cxVarp, VCMethod::COVERGROUP_SAMPLE_IFFS,
                                      {ctext(fl, "__Vcx_iffs")})));
        إذا (Ast_ڳنڍ_اظهار* ثابت iffp = crossp->iffp()) {
            m_sampleFuncp->جوڙيو_Stmtsp(جديد AstIf{fl, iffp->clone_وڻ(خطأ), samplep});
        } وإلا {
            m_sampleFuncp->جوڙيو_Stmtsp(samplep);
        }
    }

    فراغ generate_Cross_ڪوڊ(AstCoverCross* crossp) {
        UINFO(4, "  Generating code for cross: " << crossp->name());

        // Non-standard hierarchical/dotted cross item (e.g. 'cross a.b'): an implicit coverpoint
        // over the referenced expression (carried in refp->exprp()).  The grammar already warned
        // NONSTD; implicit coverpoints are not yet implemented, so generate no sampling code for
        // this cross.  When support is added the implicit coverpoint should be synthesized
        // upstream (V3LinkParse) as a real AstCoverpoint so it flows through the normal coverpoint
        // path - by here coverpoint lowering has already run.
        لكل (Ast_ڳنڍ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
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
        إذا (m_droppedCrosses.ڳڻپ(crossp)) {
            لكل (Ast_ڳنڍ* itemp = crossp->itemsp(); itemp; itemp = itemp->nextp()) {
                ثابت AstCoverpointRef* ثابت refp = VN_AS(itemp, CoverpointRef);
                إذا (m_coverpointMap.ڳوليو(refp->name()) == m_coverpointMap.پڇاڙي()) {
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

    Ast_ڳنڍ_اظهار* buildBinCondition(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp) {
        // Get the range list from the bin
        Ast_ڳنڍ* ثابت rangep = binp->rangesp();
        إذا (!rangep) أرجع خالي_اشارو;

        // Integral values resolve to the coverpoint's type, as its runtime metadata does
        ثابت منطقي integral = exprp->dtypep()->skipRefp()->isIntegralOrPacked();
        // No value form of a wildcard bin is allowed on a real coverpoint (IEEE 1800-2023 19.5.4)
        إذا (binp->isWildcard() && !integral) أرجع wildcardTypeError(binp, exprp);

        // Build condition by OR-ing all ranges together
        Ast_ڳنڍ_اظهار* ڀريل_Condp = خالي_اشارو;

        لكل (Ast_ڳنڍ* currRangep = rangep; currRangep; currRangep = currRangep->nextp()) {
            Ast_ڳنڍ_اظهار* rangeCondp = خالي_اشارو;
            currRangep = V3Const::constifyEdit(currRangep);

            إذا (AstInsideRange* irp = VN_CAST(currRangep, InsideRange)) {
                Ast_ڳنڍ_اظهار* ثابت minExprp = irp->lhsp();
                Ast_ڳنڍ_اظهار* ثابت maxExprp = irp->rhsp();
                AstConst* ثابت minConstp = VN_CAST(minExprp, Const);
                AstConst* ثابت maxConstp = VN_CAST(maxExprp, Const);
                ثابت منطقي loUnbounded = VN_IS(minExprp, Unbounded);
                ثابت منطقي hiUnbounded = VN_IS(maxExprp, Unbounded);
                إذا (loUnbounded || hiUnbounded) {
                    // Open-ended range: '$' is the coverpoint domain min/max, so the
                    // range reduces to a single inequality (e.g. {[10:$]} -> expr >= 10).
                    AstConst* ثابت boundp = hiUnbounded ? minConstp : maxConstp;
                    إذا (loUnbounded && hiUnbounded) {
                        rangeCondp = جديد AstConst{irp->fileline(), AstConst::ٻٽو_انگ_True{}};
                    } وإلا إذا (!boundp) {
                        irp->v3error("Non-constant expression in bin range; "
                                     "range bounds must be constants (IEEE 1800-2023 19.5)");
                        إذا (ڀريل_Condp) VL_DO_DANGLING(ڌڪو_Deletep(ڀريل_Condp), ڀريل_Condp);
                        أرجع خالي_اشارو;
                    } وإلا إذا (boundp->انگ().isFourState()) {
                        irp->v3error("Four-state (x/z) value in bin range bound; "
                                     "range bounds must be two-state constants");
                        إذا (ڀريل_Condp) VL_DO_DANGLING(ڌڪو_Deletep(ڀريل_Condp), ڀريل_Condp);
                        أرجع خالي_اشارو;
                    } وإلا إذا (integral) {
                        rangeCondp = build_قدر_Condition(binp, exprp, irp);
                    } وإلا {
                        rangeCondp = makeOpenRangeCondition(irp->fileline(), exprp, boundp,
                                                            /*isLowerBound=*/hiUnbounded);
                    }
                } وإلا إذا (!minConstp || !maxConstp) {
                    irp->v3error("Non-constant expression in bin range; "
                                 "range bounds must be constants (IEEE 1800-2023 19.5)");
                    إذا (ڀريل_Condp) VL_DO_DANGLING(ڌڪو_Deletep(ڀريل_Condp), ڀريل_Condp);
                    أرجع خالي_اشارو;
                } وإلا إذا (minConstp->انگ().isFourState() || maxConstp->انگ().isFourState()) {
                    irp->v3error("Four-state (x/z) value in bin range bound; "
                                 "range bounds must be two-state constants");
                    إذا (ڀريل_Condp) VL_DO_DANGLING(ڌڪو_Deletep(ڀريل_Condp), ڀريل_Condp);
                    أرجع خالي_اشارو;
                } وإلا إذا (integral) {
                    rangeCondp = build_قدر_Condition(binp, exprp, irp);
                } وإلا {
                    rangeCondp = makeRangeCondition(irp->fileline(), exprp, minExprp, maxExprp);
                }
            } وإلا إذا (AstConst* constp = VN_CAST(currRangep, Const)) {
                rangeCondp = build_قدر_Condition(binp, exprp, constp);
            } وإلا {
                currRangep->v3error("Non-constant expression in bin range; values must be "
                                    "constants (IEEE 1800-2023 19.5)");
                إذا (ڀريل_Condp) VL_DO_DANGLING(ڌڪو_Deletep(ڀريل_Condp), ڀريل_Condp);
                أرجع خالي_اشارو;
            }

            UASSERT_OBJ(rangeCondp, binp, "rangeCondp is null after building range condition");
            ڀريل_Condp
                = ڀريل_Condp ? جديد AstOr{binp->fileline(), ڀريل_Condp, rangeCondp} : rangeCondp;
        }

        أرجع ڀريل_Condp;
    }

    // Wildcard bits have no meaning for a non-integral coverpoint.
    ساكن Ast_ڳنڍ_اظهار* wildcardTypeError(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp) {
        ثابت Ast_ڳنڍ_DType* ثابت dtypep = exprp->dtypep()->skipRefp();
        exprp->v3error("Cannot use a wildcard bin on a coverpoint of type "
                       << dtypep->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.4).\n"
                       << exprp->warnContextPrimary() << '\n'
                       << binp->warnOther() << "... Location of wildcard bin\n"
                       << binp->warnContextSecondary());
        أرجع جديد AstConst{binp->fileline(), AstConst::ٻٽو_انگ_False{}};
    }

    // Match one bin value, range, or wildcard pattern.  Integral coverpoints first resolve the
    // value to their type (IEEE 1800-2023 19.5.7).  Non-owning: clones what it uses.
    Ast_ڳنڍ_اظهار* build_قدر_Condition(AstCoverBin* binp, Ast_ڳنڍ_اظهار* exprp, Ast_ڳنڍ* valuep) {
        File_سٽ* ثابت fl = valuep->fileline();
        إذا (!exprp->dtypep()->skipRefp()->isIntegralOrPacked()) {
            أرجع جديد AstEq{fl, exprp->clone_وڻ(خطأ),
                             VN_AS(valuep, ڳنڍ_اظهار)->clone_وڻ(خطأ)};
        }
        Cross_قدر_Range range{valuep, resolve_ويڪر(valuep, exprp)};
        إذا (!resolve_قدر(valuep, exprp, سچ, binp->isWildcard(), range)) {
            valuep->v3warn(E_UNSUPPORTED,
                           "Unsupported: non-integral value in a coverage bin of an "
                           "integral coverpoint.");
            أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        }
        إذا (cross_Range_خالي_جايون(range)) أرجع جديد AstConst{fl, AstConst::ٻٽو_انگ_False{}};
        AstConst* ثابت lop = new_قدر_Const(fl, range.lo, exprp);
        AstConst* ثابت hip = new_قدر_Const(fl, range.hi, exprp);
        Ast_ڳنڍ_اظهار* condp = خالي_اشارو;
        إذا (lop->انگ().isCaseEq(hip->انگ())) {
            condp = جديد AstEq{fl, exprp->clone_وڻ(خطأ), lop};
        } وإلا {
            condp = makeRangeCondition(fl, exprp, lop, hip);
            VL_DO_DANGLING(ڌڪو_Deletep(lop), lop);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(hip), hip);
        إذا (!range.wildcard) أرجع condp;
        // Match the pattern's value bits within the source-value bounds.
        V3_عدد نقاب{valuep, exprp->ويڪر()};
        V3_عدد قدر{valuep, exprp->ويڪر()};
        نقاب.op_بٽ_Non_XZ(range.نمونو_2);
        قدر.op_بٽ_One(range.نمونو_2);
        AstConst* ثابت نقاب_Constp = new_قدر_Const(fl, نقاب, exprp);
        AstConst* ثابت قدر_Constp = new_قدر_Const(fl, قدر, exprp);
        Ast_ڳنڍ_اظهار* ثابت اظهار_Masked = جديد AstAnd{fl, exprp->clone_وڻ(خطأ), نقاب_Constp};
        Ast_ڳنڍ_اظهار* ثابت قدر_Masked = جديد AstAnd{fl, قدر_Constp, نقاب_Constp->clone_وڻ(خطأ)};
        أرجع جديد AstLogAnd{fl, condp, جديد AstEq{fl, اظهار_Masked, قدر_Masked}};
    }

    فراغ generate_Coverage_Computation_ڪوڊ() {
        UINFO(4, "  Generating coverage computation code");

        // Invalidate cache: addMembersp() calls in generateCoverpointCode/generateCrossCode
        // have added new members since the last scan, so clear before re-querying.
        m_memberMap.clear();

        // get_inst_coverage(): the average of the coverpoints and crosses, weighted by their
        // option.weight (IEEE 1800-2023 19.11).  The instance node holds their runtimes.  With
        // the instances merged, get_coverage() instead, unless option.get_inst_coverage
        // (Table 19-1).
        AstFunc* ثابت getInstCoveragep
            = VN_AS(m_memberMap.ڳوليو_Member(m_covergroupp, "get_inst_coverage"), Func);
        File_سٽ* ثابت instFl = getInstCoveragep->fileline();
        AstCMethodHard* ثابت instCallp = instanceCall(instFl, VCMethod::COVERGROUP_COVERAGE);
        instCallp->dtype_سيٽ_Double();
        Ast_ڳنڍ_اظهار* instValuep = instCallp;
        إذا (m_cg_May_ملايو) {
            Ast_ڳنڍ_اظهار* ثابت mergedp = جديد AstLogAnd{
                instFl, newOptionSel(instFl, optionVar(سچ), "merge_instances", VAccess::READ),
                جديد AstLogNot{instFl, newOptionSel(instFl, optionVar(خطأ), "get_inst_coverage",
                                                   VAccess::READ)}};
            instValuep = جديد AstCond{instFl, mergedp, typeCoverageCall(instFl), instCallp};
        }
        getInstCoveragep->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            instFl, جديد AstVarRef{instFl, VN_AS(getInstCoveragep->fvarp(), Var), VAccess::WRITE},
            instValuep});

        // get_coverage(): the average of the covergroup's instances, weighted by their
        // option.weight, or with type_option.merge_instances, the coverage of the union of their
        // bins (IEEE 1800-2023 19.11.3).  Static, so the registry finds the instances.
        AstFunc* ثابت getCoveragep
            = VN_AS(m_memberMap.ڳوليو_Member(m_covergroupp, "get_coverage"), Func);
        File_سٽ* ثابت typeFl = getCoveragep->fileline();
        getCoveragep->جوڙيو_Stmtsp(جديد Ast_سونپيو{
            typeFl, جديد AstVarRef{typeFl, VN_AS(getCoveragep->fvarp(), Var), VAccess::WRITE},
            typeCoverageCall(typeFl)});
    }

    // The registry call computing the covergroup's type coverage, per its type options
    AstCMethodHard* typeCoverageCall(File_سٽ* fl) {
        AstCExpr* ثابت registryp = ctext(fl, "vlSymsp->_vm_contextp__->covergroupRegistryp()");
        registryp->dtype_سيٽ_Void();  // Opaque receiver; only ever the 'fromp' of the call below
        AstCMethodHard* ثابت callp
            = جديد AstCMethodHard{fl, registryp, VCMethod::COVERGROUP_TYPE_COVERAGE};
        callp->جوڙيو_Pinsp(ctext(fl, quoted(covergroupProtectedName())));
        callp->جوڙيو_Pinsp(newOptionSel(fl, optionVar(سچ), "weight", VAccess::READ));
        callp->جوڙيو_Pinsp(newOptionSel(fl, optionVar(سچ), "merge_instances", VAccess::READ));
        callp->جوڙيو_Pinsp(file_سٽ_Debug(m_covergroupp->fileline()));
        callp->usePtr(سچ);
        callp->dtype_سيٽ_Double();
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
        Ast_ڳنڍ_اظهار* ثابت newp = جديد AstVarRef{refp->fileline(), handleVarp, VAccess::READ};
        refp->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
    }

    فراغ rewriteVarRef(AstVarRef* refp, AstVar* handleVarp) {
        File_سٽ* ثابت fl = refp->fileline();
        // As V3LinkLValue does, the handle of a written member select is marked written
        AstMemberSel* ثابت selp
            = جديد AstMemberSel{fl, جديد AstVarRef{fl, handleVarp, refp->access()}, refp->varp()};
        selp->access(refp->access());
        refp->replaceWith(selp);
        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
    }

    فراغ rewriteFuncRef(AstFuncRef* refp, AstVar* handleVarp) {
        File_سٽ* ثابت fl = refp->fileline();
        AstArg* ثابت argsp = refp->argsp() ? refp->argsp()->unlink_Fr_پٺ_With_ايندڙ() : خالي_اشارو;
        Ast_طريقو_Call* ثابت callp = جديد Ast_طريقو_Call{
            fl, جديد AstVarRef{fl, handleVarp, VAccess::READ}, refp->name(), argsp};
        callp->taskp(refp->taskp());
        callp->dtypeFrom(refp);
        refp->replaceWith(callp);
        VL_DO_DANGLING(ڌڪو_Deletep(refp), refp);
    }

    // True if funcp is an instance method of the enclosing class or one of its bases
    منطقي isEnclosingInstanceFunc(ثابت Ast_ڳنڍ_FTask* funcp) ثابت {
        إذا (!funcp->class_طريقو() || funcp->isStatic()) أرجع خطأ;
        أرجع AstClass::isClassExtendedFrom(m_enclosingClassp, VN_AS(funcp->aboveLoopp(), Class));
    }

    منطقي isEmbeddedCovergroupVar(ثابت AstVar* varp) ثابت {
        إذا (!varp || !varp->isClassMember() || varp->isDeclTyped()) أرجع خطأ;
        ثابت AstClassRefDType* ثابت refp = VN_CAST(varp->dtypep()->skipRefp(), ClassRefDType);
        أرجع refp && refp->classp() == m_covergroupp;
    }

    AstVar* ڳوليو_Embedded_Covergroup_Var() ثابت {
        إذا (!m_enclosingClassp) أرجع خالي_اشارو;
        لكل (Ast_ڳنڍ* itemp = m_enclosingClassp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (AstVar* ثابت varp = VN_CAST(itemp, Var)) {
                إذا (isEmbeddedCovergroupVar(varp)) أرجع varp;
            }
        }
        // V3LinkParse always creates an implicit variable for an embedded covergroup.
        أرجع خالي_اشارو;  // LCOV_EXCL_LINE
    }

    std::ويڪٽر<Ast_ڳنڍ_سونپيو*> ڳوليو_Covergroup_Constructions() {
        std::ويڪٽر<Ast_ڳنڍ_سونپيو*> foundps;
        إذا (!m_embeddedVarp) أرجع foundps;
        AstFunc* ثابت enclosingNewp
            = VN_CAST(m_memberMap.ڳوليو_Member(m_enclosingClassp, "new"), Func);
        إذا (!enclosingNewp) أرجع foundps;
        enclosingNewp->foreach([&](Ast_ڳنڍ_سونپيو* asgnp) {
            ثابت AstNew* ثابت newp = VN_CAST(asgnp->rhsp(), New);
            ثابت AstVarRef* ثابت lhsRefp = VN_CAST(asgnp->lhsp(), VarRef);
            إذا (!newp || !lhsRefp || lhsRefp->varp() != m_embeddedVarp) أرجع;
            ثابت AstClassRefDType* ثابت refp = VN_CAST(newp->dtypep(), ClassRefDType);
            إذا (refp && refp->classp() == m_covergroupp) foundps.ڌڪو_پٺ(asgnp);
        });
        أرجع foundps;
    }

    std::سيٽ<ثابت AstVar*> enclosingInstanceVars() ثابت {
        std::سيٽ<ثابت AstVar*> vars;
        إذا (m_enclosingClassp) {
            m_enclosingClassp->foreachMember([&](AstClass* ثابت, AstVar* ثابت varp) {
                إذا (isEnclosingInstanceVar(varp)) vars.وجهو(varp);
            });
        }
        أرجع vars;
    }

    منطقي hasEnclosingEventRef(AstCovergroup* cgp) ثابت {
        إذا (!m_embeddedVarp || !cgp->eventp()) أرجع خطأ;
        ثابت std::سيٽ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        منطقي مليو = خطأ;
        cgp->eventp()->foreach([&](AstVarRef* refp) {
            إذا (enclosingVars.ڳڻپ(refp->varp())) مليو = سچ;
        });
        أرجع مليو;
    }

    ساكن منطقي parse_Embedded_Event_اظهار(Ast_ڳنڍ_اظهار* exprp, AstVar*& بنياد_Varp,
                                       AstVar*& memberVarp) {
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) {
            بنياد_Varp = refp->varp();
            memberVarp = خالي_اشارو;
            أرجع سچ;
        }
        AstMemberSel* ثابت selp = VN_CAST(exprp, MemberSel);
        إذا (!selp) أرجع خطأ;
        AstVarRef* ثابت بنياد_Refp = VN_CAST(selp->fromp(), VarRef);
        إذا (!بنياد_Refp) أرجع خطأ;
        بنياد_Varp = بنياد_Refp->varp();
        memberVarp = selp->varp();
        أرجع سچ;
    }

    منطقي isEventLvalue(Ast_ڳنڍ_اظهار* exprp, ثابت EmbeddedEventTrigger& trigger) ثابت {
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) exprp = selp->fromp();
        AstVar* بنياد_Varp = خالي_اشارو;
        AstVar* memberVarp = خالي_اشارو;
        إذا (!parse_Embedded_Event_اظهار(exprp, بنياد_Varp, memberVarp)) أرجع خطأ;
        أرجع بنياد_Varp == trigger.بنياد_Varp && memberVarp == trigger.memberVarp;
    }

    Ast_ڳنڍ_اظهار* newEventRead(File_سٽ* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_ڳنڍ_اظهار* ثابت basep = جديد AstVarRef{fl, trigger.بنياد_Varp, VAccess::READ};
        إذا (!trigger.memberVarp) أرجع basep;
        AstMemberSel* ثابت selp = جديد AstMemberSel{fl, basep, trigger.memberVarp};
        selp->access(VAccess::READ);
        أرجع selp;
    }

    string eventPrevName(ثابت EmbeddedEventTrigger& trigger, سائيز_t trigger_اشاريو) ثابت {
        string name = "__Vcg_prev_" + m_embeddedVarp->name() + "_" + std::منزل_string(trigger_اشاريو)
                      + "_" + trigger.بنياد_Varp->name();
        إذا (trigger.memberVarp) name += "_" + trigger.memberVarp->name();
        أرجع name;
    }

    Ast_ڳنڍ_اظهار* newEmbeddedVarNonNull(File_سٽ* fl) ثابت {
        أرجع جديد AstNeq{fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ},
                          جديد AstConst{fl, AstConst::Null{}}};
    }

    Ast_ڳنڍ_Stmt* newSampleStmt(File_سٽ* fl) ثابت {
        Ast_طريقو_Call* ثابت callp = جديد Ast_طريقو_Call{
            fl, جديد AstVarRef{fl, m_embeddedVarp, VAccess::READ}, "sample", خالي_اشارو};
        callp->taskp(m_sampleFuncp);
        callp->dtype_سيٽ_Void();
        أرجع callp->makeStmt();
    }

    فراغ installEmbeddedEventFork(Ast_Sen_وڻ* eventp,
                                  ثابت std::ويڪٽر<Ast_ڳنڍ_سونپيو*>& constructps) {
        // IEEE 1800-2023 19.3 samples coverpoints whenever their clocking event occurs. A
        // per-instance event cannot use V3Active's static sensitivity path, so spawn
        // 'fork forever begin @(event); cg.sample(); end join_none' after each construction.
        لكل (Ast_ڳنڍ_سونپيو* ثابت constructp : constructps) {
            File_سٽ* ثابت fl = constructp->fileline();
            AstLoop* ثابت loopp = جديد AstLoop{fl};
            loopp->جوڙيو_Stmtsp(جديد AstEventControl{fl, eventp->clone_وڻ(خطأ), خالي_اشارو});
            loopp->جوڙيو_Stmtsp(جديد AstIf{fl, newEmbeddedVarNonNull(fl), newSampleStmt(fl)});
            AstFork* ثابت forkp = جديد AstFork{fl, VJoinType::JOIN_NONE};
            forkp->فوري_شروعات(سچ);
            forkp->جوڙيو_Forksp(جديد AstBegin{fl, "", loopp, سچ});
            constructp->جوڙيو_ايندڙ_هتي(forkp);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(eventp), eventp);
    }

    Ast_ڳنڍ_اظهار* newEventReadyCondition(File_سٽ* fl, ثابت EmbeddedEventTrigger& trigger) ثابت {
        Ast_ڳنڍ_اظهار* ثابت curp = newEventRead(fl, trigger);
        Ast_ڳنڍ_اظهار* ثابت prevp = جديد AstVarRef{fl, trigger.prevVarp, VAccess::READ};
        Ast_ڳنڍ_اظهار* edgep = خالي_اشارو;
        // IEEE 1800-2023 9.4.2 detects edge-qualified events only on the expression's LSB,
        // while an implicit change event observes the complete expression.
        إذا (trigger.ڪنارو_Type == VEdgeType::ET_POSEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, curp, جديد AstNot{fl, prevp}}, 0, 1};
        } وإلا إذا (trigger.ڪنارو_Type == VEdgeType::ET_NEGEDGE) {
            edgep = جديد AstSel{fl, جديد AstAnd{fl, جديد AstNot{fl, curp}, prevp}, 0, 1};
        } وإلا إذا (trigger.ڪنارو_Type == VEdgeType::ET_BOTHEDGE) {
            edgep = جديد AstSel{fl, جديد AstXor{fl, curp, prevp}, 0, 1};
        } وإلا {
            edgep = جديد AstNeq{fl, curp, prevp};
        }
        أرجع جديد AstLogAnd{fl, newEmbeddedVarNonNull(fl), edgep};
    }

    std::ويڪٽر<EmbeddedEventTrigger> collectEmbeddedEventTriggers(AstCovergroup* cgp) {
        std::ويڪٽر<EmbeddedEventTrigger> triggers;
        ثابت std::سيٽ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        لكل (Ast_ڳنڍ* senp = cgp->eventp()->sensesp(); senp; senp = senp->nextp()) {
            Ast_Sen_شي* ثابت itemp = VN_AS(senp, Sen_شي);
            AstVar* بنياد_Varp = خالي_اشارو;
            AstVar* memberVarp = خالي_اشارو;
            إذا (!parse_Embedded_Event_اظهار(itemp->sensp(), بنياد_Varp, memberVarp)
                || !enclosingVars.ڳڻپ(بنياد_Varp)) {
                أرجع {};
            }
            triggers.emplace_پٺ(itemp->fileline(), بنياد_Varp, memberVarp, itemp->ڪنارو_Type());
        }
        أرجع triggers;
    }

    فراغ installEmbeddedEventTriggers(std::ويڪٽر<EmbeddedEventTrigger>& triggers,
                                      ثابت std::ويڪٽر<Ast_ڳنڍ_سونپيو*>& constructps) {
        // Without --timing, approximate a per-instance event by sampling after assignments
        // within the enclosing class. External writes and exact scheduling cannot be observed.
        إذا (constructps.خالي_جايون()) أرجع;
        لكل (سائيز_t trigger_اشاريو = 0; trigger_اشاريو < triggers.سائيز(); ++trigger_اشاريو) {
            EmbeddedEventTrigger& trigger = triggers[trigger_اشاريو];
            std::ويڪٽر<Ast_ڳنڍ_سونپيو*> assignps;
            m_enclosingClassp->foreach([&](Ast_ڳنڍ_سونپيو* asgnp) {
                إذا (isEventLvalue(asgnp->lhsp(), trigger)) assignps.ڌڪو_پٺ(asgnp);
            });
            إذا (assignps.خالي_جايون()) {
                trigger.eventFl->v3warn(
                    COVERIGN, "Unsupported: 'covergroup' clocking event signal has no assignment "
                              "within the enclosing class; no coverage sampled. Use --timing for "
                              "full support.");
                استمر;
            }
            Ast_ڳنڍ_DType* ثابت dtypep
                = trigger.memberVarp ? trigger.memberVarp->dtypep() : trigger.بنياد_Varp->dtypep();
            AstVar* ثابت prevVarp = جديد AstVar{trigger.eventFl, VVarType::MEMBER,
                                                eventPrevName(trigger, trigger_اشاريو), dtypep};
            m_enclosingClassp->جوڙيو_Membersp(prevVarp);
            trigger.prevVarp = prevVarp;
            لكل (Ast_ڳنڍ_سونپيو* ثابت asgnp : assignps) {
                File_سٽ* ثابت fl = asgnp->fileline();
                AstIf* ثابت ifp
                    = جديد AstIf{fl, newEventReadyCondition(fl, trigger), newSampleStmt(fl)};
                ifp->جوڙيو_ايندڙ_هتي(جديد Ast_سونپيو{fl,
                                               جديد AstVarRef{fl, trigger.prevVarp, VAccess::WRITE},
                                               newEventRead(fl, trigger)});
                asgnp->جوڙيو_ايندڙ_هتي(ifp);
            }
        }
    }

    فراغ delete_Coverage_شيون() {
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) {
            VL_DO_DANGLING(ڌڪو_Deletep(cpp->unlink_Fr_پٺ()), cpp);
        }
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) {
            VL_DO_DANGLING(ڌڪو_Deletep(crossp->unlink_Fr_پٺ()), crossp);
        }
        // Options not lowered: the covergroup was not processed
        لكل (Ast_Cg_Option_سونپيو* ثابت optp : m_cgOptions) {
            VL_DO_DANGLING(ڌڪو_Deletep(optp->unlink_Fr_پٺ()), optp);
        }
        m_cgOptions.clear();
    }

    صنف FormalRefVisitor final : عام VNVisitor {
        ثابت std::map<ثابت AstVar*, AstVar*>& m_replacements;

        فراغ گهمو(AstVarRef* nodep) override {
            ثابت خودڪار it = m_replacements.ڳوليو(nodep->varp());
            إذا (it == m_replacements.پڇاڙي()) أرجع;
            nodep->varp(it->ٻيو);
        }
        فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

    عام:
        واضح FormalRefVisitor(ثابت std::map<ثابت AstVar*, AstVar*>& replacements)
            : m_replacements{replacements} {}
        فراغ جاچ(Ast_ڳنڍ* nodep) { iterate(nodep); }
    };

    فراغ validateCovergroupExpressions() {
        std::سيٽ<ثابت AstVar*> sampleMembers;
        std::سيٽ<ثابت AstVar*> constructorRefMembers;
        لكل (Ast_ڳنڍ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO() || (!varp->isRef() && !varp->isConstRef())) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.ڳوليو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup constructor argument missing persistent member");
            constructorRefMembers.وجهو(varp);
            constructorRefMembers.وجهو(memberp);
        }
        لكل (Ast_ڳنڍ* stmtp = m_sampleFuncp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var);
            إذا (!varp || !varp->isIO()) استمر;
            ثابت AstVar* ثابت memberp
                = VN_CAST(m_memberMap.ڳوليو_Member(m_covergroupp, varp->name()), Var);
            UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                        "Covergroup sample argument missing persistent member");
            sampleMembers.وجهو(memberp);
        }
        Covergroup_اظهار_Valid_Visitor{sampleMembers, constructorRefMembers}.جاچ(m_constructorp);
    }

    فراغ rebindFormalRefs() {
        std::map<ثابت AstVar*, AstVar*> replacements;
        لكل (Ast_ڳنڍ* stmtp = m_constructorp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (!varp->isIO()) استمر;
                AstVar* ثابت memberp
                    = VN_CAST(m_memberMap.ڳوليو_Member(m_covergroupp, varp->name()), Var);
                UASSERT_OBJ(memberp && memberp->isClassMember(), varp,
                            "Covergroup constructor argument missing persistent member");
                replacements.emplace(varp, memberp);
            }
        }
        سائيز_t expectedBindings = 0;
        لكل (ثابت خودڪار& pair : replacements) {
            إذا (pair.پهريون->isRef() || pair.پهريون->isConstRef()) ++expectedBindings;
        }
        سائيز_t rewrittenBindings = 0;
        لكل (Ast_ڳنڍ* stmtp = m_constructorp->stmtsp(); stmtp;) {
            Ast_ڳنڍ* ثابت nextp = stmtp->nextp();
            إذا (Ast_سونپيو* ثابت assignp = VN_CAST(stmtp, سونپيو)) {
                AstVarRef* ثابت lhsp = VN_CAST(assignp->lhsp(), VarRef);
                AstVarRef* ثابت rhsp = VN_CAST(assignp->rhsp(), VarRef);
                إذا (lhsp && rhsp
                    && (lhsp->varp()->declDirection() == VDirection::REF
                        || lhsp->varp()->declDirection() == VDirection::CONSTREF)) {
                    ثابت خودڪار it = replacements.ڳوليو(rhsp->varp());
                    UASSERT_OBJ(it != replacements.پڇاڙي() && it->ٻيو == lhsp->varp(), assignp,
                                "Unexpected covergroup reference binding assignment");
                    AstCExpr* ثابت bindp = جديد AstCExpr{assignp->fileline(), ""};
                    bindp->جوڙيو(lhsp->unlink_Fr_پٺ());
                    bindp->جوڙيو(" = &");
                    bindp->جوڙيو(rhsp->unlink_Fr_پٺ());
                    assignp->replaceWith(bindp->makeStmt());
                    VL_DO_DANGLING(ڌڪو_Deletep(assignp), assignp);
                    ++rewrittenBindings;
                }
            }
            stmtp = nextp;
        }
        UASSERT_OBJ(rewrittenBindings == expectedBindings, m_constructorp,
                    "Covergroup reference argument missing binding");
        FormalRefVisitor visitor{replacements};
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) visitor.جاچ(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) visitor.جاچ(crossp);
    }

    AstVarRef* install_Enclosing_پٺ_اشارو(ثابت std::ويڪٽر<Ast_ڳنڍ_سونپيو*>& constructps) {
        // Simple-case support for embedded covergroups (IEEE 1800-2023 19.4) whose
        // coverpoints reference members of the enclosing class ("Class members can be used
        // in coverpoint expressions").  The covergroup is lowered into a sibling class with
        // no implicit handle to the enclosing object, so such references would emit
        // uncompilable C++.  Add an explicit back-pointer member to the enclosing instance,
        // route member references through it, and pass it into the constructor so
        // coverage initialization can read enclosing members. Returns an invalid
        // reference if an outer class member cannot be reached; otherwise returns an empty result.
        إذا (!m_enclosingClassp) أرجع خالي_اشارو;  // Offending refs require an enclosing class

        AstVarRef* invalidp = خالي_اشارو;
        Ast_ڳنڍ* offenderp = خالي_اشارو;
        std::سيٽ<ثابت AstVar*> ownVars;
        لكل (Ast_ڳنڍ* itemp = m_covergroupp->membersp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) ownVars.وجهو(varp);
        }
        ثابت std::سيٽ<ثابت AstVar*> enclosingVars = enclosingInstanceVars();
        std::ويڪٽر<AstVarRef*> refs_منزل_Rewrite;
        std::ويڪٽر<AstThisRef*> this_Refs_منزل_Rewrite;
        std::ويڪٽر<AstFuncRef*> func_Refs_منزل_Rewrite;
        ثابت خودڪار جاچ = [&](Ast_ڳنڍ* rootp) {
            rootp->foreach([&](AstVarRef* refp) {
                إذا (invalidp) أرجع;
                ثابت AstVar* ثابت varp = refp->varp();
                إذا (!isEnclosingInstanceVar(varp) || ownVars.ڳڻپ(varp)) أرجع;
                إذا (!enclosingVars.ڳڻپ(varp)) {
                    invalidp = refp;
                    أرجع;
                }
                refs_منزل_Rewrite.ڌڪو_پٺ(refp);
                إذا (!offenderp) offenderp = refp;
            });
            إذا (invalidp) أرجع;
            rootp->foreach([&](AstThisRef* refp) {
                ثابت AstClassRefDType* ثابت refDTypep
                    = VN_CAST(refp->dtypep()->skipRefp(), ClassRefDType);
                إذا (refDTypep && refDTypep->classp() == m_covergroupp) {
                    this_Refs_منزل_Rewrite.ڌڪو_پٺ(refp);
                    إذا (!offenderp) offenderp = refp;
                }
            });
            rootp->foreach([&](AstFuncRef* refp) {
                إذا (!isEnclosingInstanceFunc(refp->taskp())) أرجع;
                func_Refs_منزل_Rewrite.ڌڪو_پٺ(refp);
                إذا (!offenderp) offenderp = refp;
            });
        };
        لكل (AstCoverpoint* ثابت cpp : m_coverpoints) جاچ(cpp);
        لكل (AstCoverCross* ثابت crossp : m_coverCrosses) جاچ(crossp);
        لكل (Ast_Cg_Option_سونپيو* ثابت optp : m_cgOptions) جاچ(optp);
        إذا (invalidp || !offenderp) أرجع invalidp;

        UASSERT_OBJ(m_embeddedVarp, m_covergroupp, "Embedded covergroup variable not found");
        // Commit: add the back-pointer member, rewrite the references, initialize the handle.
        File_سٽ* ثابت fl = m_covergroupp->fileline();
        AstClassRefDType* ثابت enclDTypep = جديد AstClassRefDType{fl, m_enclosingClassp, خالي_اشارو};
        enclDTypep->raw_اشارو(سچ);
        v3Global.rootp()->typeTablep()->جوڙيو_Typesp(enclDTypep);
        AstVar* ثابت handleVarp
            = جديد AstVar{fl, VVarType::MEMBER, "__Vcg_enclosingp", enclDTypep};
        m_covergroupp->جوڙيو_Membersp(handleVarp);
        AstVar* ثابت argumentp = جديد AstVar{fl, VVarType::BLOCKTEMP, "__Vcg_parentp", enclDTypep};
        argumentp->direction(VDirection::داخلا);
        argumentp->declDirection(VDirection::داخلا);
        argumentp->funcLocal(سچ);
        argumentp->noReset(سچ);
        argumentp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        m_constructorp->جوڙيو_Stmtsp(argumentp);
        m_constructorp->prependStmtsp(جديد Ast_سونپيو{fl, memberRef(fl, handleVarp, VAccess::WRITE),
                                                    جديد AstVarRef{fl, argumentp, VAccess::READ}});

        // Route each enclosing-member reference through the back-pointer: 'm' -> 'h.m'.
        لكل (AstVarRef* ثابت refp : refs_منزل_Rewrite) { rewriteVarRef(refp, handleVarp); }
        لكل (AstThisRef* ثابت refp : this_Refs_منزل_Rewrite) { rewriteThisRef(refp, handleVarp); }
        لكل (AstFuncRef* ثابت refp : func_Refs_منزل_Rewrite) { rewriteFuncRef(refp, handleVarp); }

        // Append a named hidden argument to preserve positional and defaulted user arguments.
        لكل (Ast_ڳنڍ_سونپيو* ثابت constructp : constructps) {
            File_سٽ* ثابت cfl = constructp->fileline();
            AstCExpr* ثابت thisp = جديد AstCExpr{cfl, "this"};
            thisp->dtypep(enclDTypep);
            VN_AS(constructp->rhsp(), New)->جوڙيو_Argsp(جديد AstArg{cfl, argumentp->name(), thisp});
        }
        أرجع خالي_اشارو;
    }

    فراغ گهمو(AstClass* nodep) override {
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
            m_embeddedVarp = ڳوليو_Embedded_Covergroup_Var();
            m_covergroupName = covergroupTypeName();
            m_sampleFuncp = خالي_اشارو;
            m_constructorp = خالي_اشارو;
            std::ويڪٽر<EmbeddedEventTrigger> embeddedEventTriggers;
            Ast_Sen_وڻ* embeddedEventForkp = خالي_اشارو;

            // Extract and store the clocking event from AstCovergroup node
            // The parser creates this node to preserve the event information
            منطقي hasUnsupportedEvent = خطأ;
            لكل (Ast_ڳنڍ* itemp = nodep->membersp(); itemp;) {
                Ast_ڳنڍ* ثابت nextp = itemp->nextp();
                إذا (AstCovergroup* ثابت cgp = VN_CAST(itemp, Covergroup)) {
                    // Store the event in the global map for V3Active to retrieve later
                    // V3LinkParse only creates this sentinel AstCovergroup node when a clocking
                    // event exists, so cgp->eventp() is always non-null here.
                    UASSERT_OBJ(cgp->eventp(), cgp,
                                "Sentinel AstCovergroup in class must have non-null eventp");
                    إذا (hasEnclosingEventRef(cgp)) {
                        UASSERT_OBJ(m_embeddedVarp, cgp,
                                    "Embedded covergroup event has no instance variable");
                        إذا (v3Global.opt.timing().is_سيٽ_True()) {
                            embeddedEventForkp = cgp->eventp()->unlink_Fr_پٺ();
                        } وإلا {
                            embeddedEventTriggers = collectEmbeddedEventTriggers(cgp);
                            إذا (embeddedEventTriggers.خالي_جايون()) {
                                cgp->v3warn(COVERIGN,
                                            "Unsupported: 'covergroup' clocking event on complex "
                                            "member expression; use --timing for full support.");
                                hasUnsupportedEvent = سچ;
                            }
                        }
                        VL_DO_DANGLING(ڌڪو_Deletep(cgp->unlink_Fr_پٺ()), cgp);
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
            m_sampleFuncp = VN_CAST(m_memberMap.ڳوليو_Member(nodep, "sample"), Func);
            // V3LinkParse always synthesizes a sample() method for every covergroup, and the
            // sampling-code generation below dereferences m_sampleFuncp unconditionally.
            UASSERT_OBJ(m_sampleFuncp, nodep, "Covergroup missing synthesized sample() method");
            m_sampleFuncp->isCovergroupSample(سچ);
            m_constructorp = VN_CAST(m_memberMap.ڳوليو_Member(nodep, "new"), Func);
            UINFO(9, "Found sample() method: " << (m_sampleFuncp ? "yes" : "no"));
            UINFO(9, "Found constructor: " << (m_constructorp ? "yes" : "no"));

            // If covergroup has unsupported clocking event, skip processing it
            // but still clean up coverpoints so they don't reach downstream passes
            إذا (hasUnsupportedEvent) {
                iterateChildren(nodep);
                validateCovergroupExpressions();
                rebindFormalRefs();
                delete_Coverage_شيون();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(ڌڪو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }

            iterateChildren(nodep);
            validateCovergroupExpressions();
            rebindFormalRefs();
            ثابت std::ويڪٽر<Ast_ڳنڍ_سونپيو*> constructps = ڳوليو_Covergroup_Constructions();

            // Embedded covergroups (IEEE 1800-2023 19.4): coverpoints, iff expressions, and
            // crosses may reference members of the enclosing class. The covergroup is lowered
            // into a sibling class with no implicit handle to the enclosing instance. Install
            // an explicit back-pointer and route the references through it.
            إذا (AstVarRef* ثابت invalidp = install_Enclosing_پٺ_اشارو(constructps)) {
                invalidp->v3error("Non-static member "
                                  << invalidp->varp()->prettyNameQ()
                                  << " of an outer class requires an explicit "
                                     "object handle (IEEE 1800-2023 8.23).");
                delete_Coverage_شيون();
                إذا (embeddedEventForkp) {
                    VL_DO_DANGLING(ڌڪو_Deletep(embeddedEventForkp), embeddedEventForkp);
                }
                أرجع;
            }
            installEmbeddedEventTriggers(embeddedEventTriggers, constructps);
            إذا (embeddedEventForkp) installEmbeddedEventFork(embeddedEventForkp, constructps);
            processCovergroup();
            // Remove lowered coverpoints/crosses from the class - they have been
            // fully translated into C++ code and must not reach downstream passes
            delete_Coverage_شيون();
        } وإلا {
            // Track the lexically enclosing class so a nested covergroup can resolve
            // references to the enclosing object's members (installEnclosingBackPointer).
            VL_RESTORER(m_enclosingClassp);
            m_enclosingClassp = nodep;
            iterateChildren(nodep);
        }
    }

    فراغ گهمو(AstCoverpoint* nodep) override {
        UINFO(9, "Found coverpoint: " << nodep->name());
        m_coverpoints.ڌڪو_پٺ(nodep);
        m_coverpointMap.emplace(nodep->name(), nodep);
        iterateChildren(nodep);
    }

    فراغ گهمو(AstCoverCross* nodep) override {
        UINFO(9, "Found cross: " << nodep->name());
        m_coverCrosses.ڌڪو_پٺ(nodep);
        iterateChildren(nodep);
    }

    // V3Width leaves only the covergroup-level options lowerCovergroupOptions() takes
    فراغ گهمو(Ast_Cg_Option_سونپيو* nodep) override { m_cgOptions.ڌڪو_پٺ(nodep); }

    // A package, interface, or module, so the design unit declaring the classes within
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        VL_RESTORER(m_unitp);
        m_unitp = nodep;
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    FunctionalCoverageVisitor(AstNetlist* nodep,
                              ثابت std::سيٽ<ثابت AstVar*>& mergeableTypeOptions)
        : m_mergeableTypeOptions{mergeableTypeOptions} {
        iterate(nodep);
    }
    ~FunctionalCoverageVisitor() override = افتراضي;
};

//######################################################################
// Functional coverage class functions

فراغ V3Covergroup::covergroup(AstNetlist* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    ثابت Covergroup_سونپيو_Valid_Visitor validVisitor{nodep};
    إذا (!validVisitor.valid()) V3Error::abortIfErrors();
    {  // Destruct before checking
        FunctionalCoverageVisitor{nodep, validVisitor.mergeableTypeOptions()};
    }
    V3Global::dump_Check_Global_وڻ("coveragefunc", 0, dump_وڻ_Either_Level() >= 3);
}
