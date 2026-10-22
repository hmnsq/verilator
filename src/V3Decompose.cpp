// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split arrays and structs into separate variables
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
// V3Decompose replaces variables of aggregate type (unpacked array, unpacked
// struct, multidimensional packed array, or packed struct) with one variable
// per element or member, called a component. This avoids false combinational
// loops (UNOPTFLAT) through the parts of a variable, and lets downstream passes
// optimize each component separately. Note specifically that one dimensional
// packed arrays (vectors, e.g. logic [31:0]) are not split into individual
// bits by this pass.
//
// Components that are themselves aggregates can be split further. The pass
// decides for the variable, and for each of its components at any depth,
// whether to split it.
//
// The pass proceeds in three steps:
//   - DecomposeRecord walks the netlist and records necessary information about
//     accesses to aggregate variables, without changing the tree.
//   - DecomposeDecision decides which variables and components to split, in one
//     iterative analysis of the data structures recorded by DecomposeRecord.
//   - DecomposeRewrite replaces references to, and expands assignments between
//     split variables and components, in a single rewrite across all constructs
//     involved.
// This structure ensures there is only a single traversal of the netlist, the
// complete splitting decision is made globally, independent of iteration order,
// and the rewriting is done after all decisions are made, so there is no need
// to re-analyze, or keep track of stale references anywhere.
//
// The core concept and data structure that enables the decomposition of the
// three steps is the 'place'. A 'place' represents a possible storage location:
// - either an original variable,
// - or one of its components, recursively.
// 'places' form a tree, with the original variable as the root. For example,
// the tree of 'places' involved for the aggregate variable 's' are:
//
//   struct packed {
//     logic [1:0][3:0] m;
//     logic [2:0] f;
//   } s [2];
//
//   s                      'place' for the unpacked array of 2 packed structs
//   +-- s[0]               'place' for element 0 of 's', a packed struct
//   |   +-- s[0].m         'place' for member 'm' of 's[0]', a packed array of 2 elements
//   |   |   +-- s[0].m[0]  'place' logic [3:0], a non splittable leaf
//   |   |   +-- s[0].m[1]  'place' logic [3:0], a non splittable leaf
//   |   +-- s[0].f         'place' logic [2:0], a non splittable leaf
//   +-- s[1]               Same subtree structure as 's[0]'
//       +-- s[1].m
//       |   +-- s[1].m[0]
//       |   +-- s[1].m[1]
//       +-- s[1].f
//
// A place is automatically split if it can be, and it wants to be.
// It can be split if all of the following are true:
//   - it is an aggregate variable
//   - the original variable is eligible for splitting (not public, etc.)
//   - its parent is split (if it is a component)
//   - it is not blocked, by being referenced other than in splittable constructs
// It wants to be split if any of the following are true:
//   - one of its components is addressed by a constant select
//   - the original variable is marked with split_var
//   - a copy assignment between it and another split place exists
// Note specifically that a reference outside a constant indexed select, or
// a copy assignment will block splitting, so effectively the variable is split
// mostly if it is only accessed via constant indexed selects, and it's only
// assigned whole (recursively for its components).
//
// E.g. this will split 'arr' into 'arr[0]' and 'arr[1]':
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[1];  // All indices are constant, so 'arr' is split
//
// This will not split 'arr', as it is indexed with a non-constant index:
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[i];  // 'i' is not a constant, so 'arr' is not split
//
// Copy assignments propagate splitting. An assignment between two places is
// recorded as a copy. When a place is split, its copies are split at the
// boundaries of its components. For example:
//
//   typedef struct packed {
//     logic [3:0] hi;
//     logic [3:0] lo;
//   } pair_t;
//   pair_t a, b;
//   assign b = {a.lo + 4'd1, in};
//   assign a = b;
//   assign out = a.hi;
//
// 'a' is split, as its components are selected. This splits the 'a = b' copy
// into copies between 'a.hi' and 'b[7:4]', and 'a.lo' and 'b[3:0]', each covering
// only one component of 'b', so 'b' is split too. The result is a list of
// assignments between the components of 'a' and 'b':
//
//   assign b__DOT__hi = a__DOT__lo + 4'd1;
//   assign b__DOT__lo = in;
//   assign a__DOT__hi = b__DOT__hi;
//   assign a__DOT__lo = b__DOT__lo;
//   assign out = a__DOT__hi;
//
// Which later passes can simplify into 'out = in + 4'd1', with intermediate
// variables discarded when possible.
//
//*************************************************************************

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Decompose.h"

#अन्तर्भाव "V3AstUserAllocator.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

वर्ग DecomposeBase VL_NOT_FINAL {
संरक्षित:
    // TYPES

    // A component (element or member) of an aggregate type
    संरचना Component final {
        Ast_गाठ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        पूर्णांक declIdx;  // The declared index of the element, if an array
        पूर्णांक lsb;  // LSB in the whole value, if packed
        पूर्णांक msb;  // MSB in the whole value, if packed
    };

    संरचना Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    संरचना Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        पूर्णांक lsb;  // The first bit covered of this end
        पूर्णांक otherLsb;  // The first bit covered of the other end
        पूर्णांक width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    संरचना Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_गाठ_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        तार्किक splittable = सत्य;  // Can be split (there is no reason not to)
        तार्किक wantsSplit = असत्य;  // Should be split (if possible, do split)
        तार्किक split = असत्य;  // Place was split
        तार्किक queued = असत्य;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    संरचना Select final {
        Ast_गाठ_अभिव्यक्ती* exprp;  // The select
        Place* placep;  // The component
        पूर्णांक lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    संरचना Assignment final {
        Ast_गाठ_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

सार्वजनिक:
    // The state of the pass, shared by the steps
    वर्ग State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        स्थिरांक VNUser4InUse m_user4InUse;

    सार्वजनिक:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_गाठ_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

संरक्षित:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    स्थिर तार्किक isUnpacked(स्थिरांक Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        जर (VN_IS(dtypep, UnpackArrayDType)) परत सत्य;
        स्थिरांक AstStructDType* स्थिरांक structp = VN_CAST(dtypep, StructDType);
        परत structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    स्थिर तार्किक isPacked(स्थिरांक Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        जर (स्थिरांक AstPackArrayDType* स्थिरांक arrayp = VN_CAST(dtypep, PackArrayDType)) {
            परत arrayp->subDTypep()->width() > 1;
        }
        स्थिरांक AstStructDType* स्थिरांक structp = VN_CAST(dtypep, StructDType);
        परत structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    स्थिर तार्किक isSplittableType(स्थिरांक Ast_गाठ_DType* dtypep) {
        जर (isPacked(dtypep)) परत v3Global.opt.fDecomposePacked();
        जर (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            जर (स्थिरांक AstStructDType* स्थिरांक structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                साठी (स्थिरांक AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    जर (mp->valuep()) परत असत्य;
                }
            }
            परत v3Global.opt.fDecomposeUnpacked();
        }
        परत असत्य;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    स्थिर Ast_गाठ_DType* dtypeOf(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) परत refp->varp()->dtypep();
        परत nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    स्थिरांक std::vector<Component>& dtypeComponents(Ast_गाठ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        जर (compsr.empty()) {
            जर (स्थिरांक Ast_गाठ_Array_DType* स्थिरांक arrayp = VN_CAST(dtypep, गाठ_Array_DType)) {
                स्थिरांक तार्किक packed = isPacked(dtypep);
                स्थिरांक VNumRange range = arrayp->declRange();
                स्थिरांक तार्किक rev = packed && range.ascending();  // Only for naming components
                Ast_गाठ_DType* स्थिरांक subp = arrayp->subDTypep();
                साठी (पूर्णांक i = 0; i < range.elements(); ++i) {
                    स्थिरांक पूर्णांक lsb = packed ? i * subp->width() : 0;
                    स्थिरांक पूर्णांक msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    स्थिरांक पूर्णांक declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ढकला_back({subp, nullptr, declIdx, lsb, msb});
                }
            } अन्यथा {
                स्थिरांक AstStructDType* स्थिरांक structp = VN_AS(dtypep, StructDType);
                स्थिरांक तार्किक packed = isPacked(dtypep);
                साठी (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    स्थिरांक पूर्णांक lsb = packed ? mp->lsb() : 0;
                    स्थिरांक पूर्णांक msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.ढकला_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        परत compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    माप_प्रकार componentIndex(Ast_गाठ_DType* dtypep, पूर्णांक bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        स्थिरांक std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        स्थिरांक स्वतः it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](स्थिरांक Component& comp, पूर्णांक b) {  //
                                             परत b > comp.msb;
                                         });
        परत it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, माप_प्रकार idx) {
        स्थिरांक std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        जर (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        जर (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        परत childpr.get();
    }

    // Is 'nodep' cheap to clone
    स्थिर तार्किक isCheap(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        // Constants are cheap
        जर (VN_IS(nodep, Const)) परत सत्य;
        // So are resets
        जर (VN_IS(nodep, CReset)) परत सत्य;
        // Variable references are cheap for the most part
        जर (स्थिरांक AstVarRef* स्थिरांक refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            जर (refp->varp()->isForced()) परत असत्य;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            जर (refp->varp()->isSc()) परत असत्य;
            // Otherwise the path rooted here is cheap
            परत सत्य;
        }
        // Bit selects only with a constant LSB
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
            परत VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        जर (स्थिरांक AstArraySel* स्थिरांक selp = VN_CAST(nodep, ArraySel)) {
            परत isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        जर (स्थिरांक AstStructSel* स्थिरांक selp = VN_CAST(nodep, StructSel)) {  //
            परत isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        जर (स्थिरांक AstExtend* स्थिरांक extp = VN_CAST(nodep, Extend)) {  //
            परत isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        परत असत्य;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

वर्ग DecomposeRecord final : सार्वजनिक VNVisitorConst, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    स्थिरांक VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_गाठ_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warnNoSplit(AstVar* varp, स्थिरांक Ast_गाठ* wherep, स्थिरांक अक्षर* reasonp) {
        // Only warn if user requested splitting
        जर (!varp->attrSplitVar()) परत;
        // A packed variable might still be split by V3BitBlast, which warns if not
        जर (isPacked(varp->dtypep())) परत;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, सत्य);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        जर (!varp->user1()) {
            स्थिरांक तार्किक eligible = [&]() {
                // Check data type
                जर (!isSplittableType(varp->dtypep())) परत असत्य;
                // Check properties
                स्थिरांक अक्षर* स्थिरांक reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                जर (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                परत !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        परत varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(स्थिरांक AstVarRef* refp) {
        AstVarScope* स्थिरांक vscp = refp->varScopep();
        जर (!isEligible(vscp->varp())) परत nullptr;
        Place& place = m_state.places(vscp);
        जर (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.ढकला_back(&place);
        }
        परत &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    स्थिर शून्य block(Place* placep, स्थिरांक Ast_गाठ* wherep) {
        placep->splittable = असत्य;
        // Warn on the variable only, a component is split as far as possible, as requested
        जर (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_गाठ_अभिव्यक्ती* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        जर (AstArraySel* स्थिरांक selp = VN_CAST(exprp, ArraySel)) {
            जर (Place* स्थिरांक fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                स्थिरांक AstConst* स्थिरांक bitp = VN_CAST(selp->bitp(), Const);
                जर (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* स्थिरांक childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = सत्य;
                    chain.ढकला_back({selp, childp, 0});
                    परत childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            परत nullptr;
        }
        // Struct selects correspond one to one with a component select
        जर (AstStructSel* स्थिरांक selp = VN_CAST(exprp, StructSel)) {
            जर (Place* स्थिरांक fromp = resolveChain(selp->fromp(), chain)) {
                जर (!isUnpacked(fromp->dtypep)) परत nullptr;
                AstStructDType* स्थिरांक structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                जर (!structp->user1SetOnce()) {
                    स्थिरांक std::vector<Component>& compsr = dtypeComponents(structp);
                    साठी (माप_प्रकार i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                स्थिरांक Ast_गाठ* स्थिरांक memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* स्थिरांक childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = सत्य;
                chain.ढकला_back({selp, childp, 0});
                परत childp;
            }
            परत nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        जर (AstSel* स्थिरांक selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            जर (!placep) {
                iterateConst(selp->lsbp());
                परत nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(selp->lsbp(), Const);
            जर (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                परत nullptr;
            }
            // Descend into the Place containing the bits, until exact
            पूर्णांक lsb = lsbp->toSInt();
            पूर्णांक msb = lsb + selp->widthConst() - 1;
            पर्यंत (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                जर (!isPacked(placep->dtypep)) परत nullptr;
                स्थिरांक माप_प्रकार idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                जर (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    परत nullptr;
                }
                placep->wantsSplit = सत्य;
                स्थिरांक Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.ढकला_back({selp, placep, lsb});
            }
            परत placep;
        }
        // Base case
        जर (AstVarRef* स्थिरांक refp = VN_CAST(exprp, VarRef)) परत placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        परत nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    शून्य referencedWhole(Ast_गाठ_अभिव्यक्ती* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        जर (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            परत;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    शून्य visitSelect(Ast_गाठ_अभिव्यक्ती* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* स्थिरांक placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        जर (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        जर (!chain.empty()) m_state.chains.ढकला_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    स्थिर तार्किक hasReversedRange(स्थिरांक Ast_गाठ_DType* aDTypep, स्थिरांक Ast_गाठ_DType* bDTypep) {
        स्थिरांक AstUnpackArrayDType* स्थिरांक aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        स्थिरांक AstUnpackArrayDType* स्थिरांक bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        जर (!aArrayp || !bArrayp) परत असत्य;
        जर (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) परत सत्य;
        परत hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    शून्य visitAssignment(Ast_गाठ_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        स्थिरांक तार्किक reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        जर (!m_assignp) परत;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = nodep->lhsp();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = nodep->rhsp();
        Place* स्थिरांक lPlacep = lhsp->user1u().to<Place*>();
        Place* स्थिरांक rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        स्थिरांक तार्किक splittable = [&]() {
            // Assignment between two Places is splittable
            जर (lPlacep && rPlacep) परत सत्य;
            // If the LHS is a Place, the RHS must be a splittable expression
            जर (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                जर (isPacked(lPlacep->dtypep)) परत सत्य;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                परत isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            जर (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                जर (isPacked(rPlacep->dtypep)) परत सत्य;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                परत isCheap(lhsp);
            }
            // Otherwise not splittable
            परत असत्य;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        जर (!splittable) {
            जर (rPlacep) block(rPlacep, rhsp);
            जर (lPlacep) block(lPlacep, lhsp);
            परत;
        }

        // If both sides are places, record the whole copies for the Decision phase
        जर (lPlacep && rPlacep) {
            स्थिरांक पूर्णांक width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.ढकला_back({rPlacep, 0, 0, width});
            rPlacep->copies.ढकला_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.ढकला_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    शून्य visit(Ast_गाठ_DType*) override {}  // No references in data types
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    शून्य visit(AstArraySel* nodep) override { visitSelect(nodep); }
    शून्य visit(AstStructSel* nodep) override { visitSelect(nodep); }
    शून्य visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    शून्य visit(AstAssign* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    शून्य visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    शून्य visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        जर (Place* स्थिरांक placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    शून्य visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* स्थिरांक varp = nodep->varp();
        जर (!isEligible(varp)) परत;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        साठी (Place* स्थिरांक placep : m_state.rootps) {
            जर (!isEligible(placep->vscp->varp())) placep->splittable = असत्य;
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

वर्ग DecomposeDecision final : सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    स्थिरांक VNUser1InUse m_user1InUse;

    // STATE
    std::vector<Place*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::vector<AstVar*>> m_splitVarps;
    VDouble0 m_statSplitUnpackedOrig;  // Original AstVars of unpacked type split
    VDouble0 m_statSplitPackedOrig;  // Original AstVars of packed type split
    VDouble0 m_statSplitUnpackedComp;  // Component AstVars of unpacked type split
    VDouble0 m_statSplitPackedComp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    स्थिर तार्किक canSplit(स्थिरांक Place* placep) {
        // Not if not splittable
        जर (!placep->splittable) परत असत्य;
        // Not if already split
        जर (placep->split) परत असत्य;
        // Original variable or its parent was already split
        परत !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    शून्य enqueue(Place* placep) {
        जर (placep->queued || !placep->wantsSplit || !canSplit(placep)) परत;
        placep->queued = सत्य;
        m_worklist.ढकला_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    शून्य createComponents(Place* placep) {
        AstVarScope* स्थिरांक vscp = placep->vscp;
        AstVar* स्थिरांक varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        जर (varps.empty()) {
            स्थिरांक तार्किक packed = isPacked(varp->dtypep());
            जर (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } अन्यथा {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_ओळ* स्थिरांक flp = varp->fileline();
            स्थिरांक VVarType varType = varp->varType();
            स्थिरांक std::string name = varp->name();
            साठी (स्थिरांक Component& comp : dtypeComponents(varp->dtypep())) {
                स्थिरांक std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_गाठ::encode_संख्या(comp.declIdx) + "__KET__";
                AstVar* स्थिरांक newp = नवीन AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.ढकला_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* स्थिरांक scopep = vscp->scopep();
        File_ओळ* स्थिरांक flp = vscp->fileline();
        साठी (माप_प्रकार i = 0; i < varps.size(); ++i) {
            AstVarScope* स्थिरांक newp = नवीन AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    शून्य addCopy(Place* placep, स्थिरांक Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        जर (!placep->splittable) परत;
        // Add the copy to the Place
        placep->copies.ढकला_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        जर (!isPacked(placep->dtypep)) परत;
        जर (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = सत्य;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    शून्य newCopy(Place* ap, पूर्णांक aLsb, Place* bp, पूर्णांक bLsb, पूर्णांक width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    शून्य splitCopy(Place* placep, स्थिरांक Copy& copy) {
        Place* स्थिरांक otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        जर (isPacked(placep->dtypep)) {
            स्थिरांक std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            स्थिरांक पूर्णांक lsb = copy.lsb;
            स्थिरांक पूर्णांक msb = copy.lsb + copy.width - 1;
            स्थिरांक माप_प्रकार first = componentIndex(placep->dtypep, lsb);
            साठी (माप_प्रकार i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                स्थिरांक Component& comp = comps[i];
                स्थिरांक पूर्णांक partLsb = std::max(lsb, comp.lsb);
                स्थिरांक पूर्णांक partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            परत;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = सत्य;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        स्थिरांक माप_प्रकार size = dtypeComponents(placep->dtypep).size();
        साठी (माप_प्रकार i = 0; i < size; ++i) {
            Place* स्थिरांक ap = componentOf(placep, i);
            जर (!ap->splittable) सुरूठेवा;
            Place* स्थिरांक bp = componentOf(otherp, i);
            जर (!bp->splittable) सुरूठेवा;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    शून्य splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = सत्य;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        स्थिरांक माप_प्रकार size = dtypeComponents(placep->dtypep).size();
        साठी (माप_प्रकार i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        साठी (स्थिरांक Copy& copy : placep->copies) {
            जर (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        साठी (Place* स्थिरांक placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        पर्यंत (!m_worklist.empty()) {
            Place* स्थिरांक placep = m_worklist.back();
            m_worklist.काढा_back();
            placep->queued = असत्य;
            splitPlace(placep);
        }

        V3Stats::addStat("Optimizations, Decompose, unpacked variables split",
                         m_statSplitUnpackedOrig);
        V3Stats::addStat("Optimizations, Decompose, packed variables split",
                         m_statSplitPackedOrig);
        V3Stats::addStat("Optimizations, Decompose, unpacked components split further",
                         m_statSplitUnpackedComp);
        V3Stats::addStat("Optimizations, Decompose, packed components split further",
                         m_statSplitPackedComp);
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

वर्ग DecomposeRewrite final : सार्वजनिक VNDeleter, सार्वजनिक DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    स्थिरांक VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    स्थिर AstVarRef* newRef(File_ओळ* flp, स्थिरांक Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        परत नवीन AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    शून्य rewriteChain(स्थिरांक std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        स्थिरांक Select* selectp = nullptr;
        साठी (स्थिरांक Select& select : chain) {
            जर (select.placep->parentp->split) selectp = &select;
        }
        जर (!selectp) परत;
        Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        स्थिरांक VAccess access = [&]() {
            स्थिरांक Ast_गाठ_अभिव्यक्ती* rootp = chain.front().exprp;
            पर्यंत (सत्य) {
                जर (स्थिरांक AstArraySel* स्थिरांक selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } अन्यथा जर (स्थिरांक AstStructSel* स्थिरांक selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } अन्यथा जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } अन्यथा {
                    परत VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_ओळ* स्थिरांक flp = exprp->fileline();
        Ast_गाठ_अभिव्यक्ती* newp = newRef(flp, selectp->placep, access);
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(exprp, Sel)) {
            स्थिरांक पूर्णांक width = selp->widthConst();
            जर (width != newp->width()) newp = नवीन AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ढकला_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_गाठ_अभिव्यक्ती* newSelect(Ast_गाठ_अभिव्यक्ती* fromp, Ast_गाठ_DType* dtypep, माप_प्रकार idx) {
        File_ओळ* स्थिरांक flp = fromp->fileline();
        स्थिरांक Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        जर (isPacked(dtypep)) {
            AstSel* स्थिरांक selp = नवीन AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            परत selp;
        }
        // If unpacked array, it's an ArraySel
        जर (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            स्थिरांक पूर्णांक i = static_cast<पूर्णांक>(idx);
            UASSERT_OBJ(static_cast<माप_प्रकार>(i) == idx, fromp, "Index overflow");
            परत नवीन AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* स्थिरांक selp = नवीन AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        परत selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    स्थिर std::vector<Ast_गाठ_अभिव्यक्ती*> concatTerms(Ast_गाठ_अभिव्यक्ती* nodep) {
        std::vector<Ast_गाठ_अभिव्यक्ती*> termps;
        std::vector<Ast_गाठ_अभिव्यक्ती*> चळत{nodep};
        पर्यंत (!चळत.empty()) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक exprp = चळत.back();
            चळत.काढा_back();
            जर (AstConcat* स्थिरांक concatp = VN_CAST(exprp, Concat)) {
                चळत.ढकला_back(concatp->lhsp());
                चळत.ढकला_back(concatp->rhsp());
                सुरूठेवा;
            }
            जर (AstReplicate* स्थिरांक repp = VN_CAST(exprp, Replicate)) {
                स्थिरांक AstConst* स्थिरांक countp = VN_AS(repp->countp(), Const);
                साठी (uint32_t i = 0; i < countp->toUInt(); ++i) चळत.ढकला_back(repp->srcp());
                सुरूठेवा;
            }
            termps.ढकला_back(exprp);
        }
        परत termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    स्थिर तार्किक isSliceable(स्थिरांक Ast_गाठ_अभिव्यक्ती* nodep) {
        जर (स्थिरांक AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
            परत VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        जर (स्थिरांक AstConcat* स्थिरांक catp = VN_CAST(nodep, Concat)) {
            परत isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        जर (स्थिरांक AstCond* स्थिरांक condp = VN_CAST(nodep, Cond)) {
            परत isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        जर (स्थिरांक AstAnd* स्थिरांक andp = VN_CAST(nodep, And)) {
            परत isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        जर (स्थिरांक AstOr* स्थिरांक orp = VN_CAST(nodep, Or)) {
            परत isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        जर (स्थिरांक AstXor* स्थिरांक xorp = VN_CAST(nodep, Xor)) {
            परत isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        जर (स्थिरांक AstExtend* स्थिरांक extp = VN_CAST(nodep, Extend)) {
            परत isSliceable(extp->lhsp());
        }
        जर (स्थिरांक AstNot* स्थिरांक notp = VN_CAST(nodep, Not)) {  //
            परत isSliceable(notp->lhsp());
        }
        जर (स्थिरांक AstReplicate* स्थिरांक repp = VN_CAST(nodep, Replicate)) {
            परत isSliceable(repp->srcp());
        }
        परत isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    स्थिर Ast_गाठ_अभिव्यक्ती* newSlice(Ast_गाठ_अभिव्यक्ती* nodep, पूर्णांक lsb, पूर्णांक width) {
        File_ओळ* स्थिरांक flp = nodep->fileline();
        // A constant, as a new constant of the bits
        जर (स्थिरांक AstConst* स्थिरांक constp = VN_CAST(nodep, Const)) {
            V3_संख्या अंक{nodep, width};
            अंक.opSel(constp->अंक(), lsb + width - 1, lsb);
            परत नवीन AstConst{flp, अंक};
        }
        // A constant bit select, from the bits of what it selects from
        जर (AstSel* स्थिरांक selp = VN_CAST(nodep, Sel)) {
            जर (स्थिरांक AstConst* स्थिरांक lsbp = VN_CAST(selp->lsbp(), Const)) {
                परत newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        जर (AstConcat* स्थिरांक catp = VN_CAST(nodep, Concat)) {
            स्थिरांक पूर्णांक rWidth = catp->rhsp()->width();
            स्थिरांक पूर्णांक msb = lsb + width - 1;
            जर (msb < rWidth) परत newSlice(catp->rhsp(), lsb, width);
            जर (lsb >= rWidth) परत newSlice(catp->lhsp(), lsb - rWidth, width);
            परत नवीन AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        जर (AstCond* स्थिरांक condp = VN_CAST(nodep, Cond)) {
            परत नवीन AstCond{flp, condp->condp()->clone_वृक्ष_Pure(असत्य),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        जर (AstAnd* स्थिरांक andp = VN_CAST(nodep, And)) {
            परत नवीन AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        जर (AstOr* स्थिरांक orp = VN_CAST(nodep, Or)) {
            परत नवीन AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        जर (AstXor* स्थिरांक xorp = VN_CAST(nodep, Xor)) {
            परत नवीन AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        जर (AstExtend* स्थिरांक extp = VN_CAST(nodep, Extend)) {
            स्थिरांक पूर्णांक srcWidth = extp->lhsp()->width();
            स्थिरांक पूर्णांक msb = lsb + width - 1;
            जर (msb < srcWidth) परत newSlice(extp->lhsp(), lsb, width);
            जर (lsb >= srcWidth) परत नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, width, 0};
            परत नवीन AstConcat{
                flp, नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        जर (AstNot* स्थिरांक notp = VN_CAST(nodep, Not)) {
            परत नवीन AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        जर (AstReplicate* स्थिरांक repp = VN_CAST(nodep, Replicate)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक srcp = repp->srcp();
            स्थिरांक पूर्णांक srcWidth = srcp->width();
            स्थिरांक पूर्णांक msb = lsb + width - 1;
            Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
            साठी (पूर्णांक copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                स्थिरांक पूर्णांक partLsb = std::max(lsb, copyLsb) - copyLsb;
                स्थिरांक पूर्णांक partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_गाठ_अभिव्यक्ती* स्थिरांक bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? नवीन AstConcat{flp, bitsp, resultp} : bitsp;
            }
            परत resultp;
        }
        Ast_गाठ_अभिव्यक्ती* स्थिरांक clonep = nodep->clone_वृक्ष_Pure(असत्य);
        जर (lsb == 0 && width == nodep->width()) परत clonep;
        परत नवीन AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_गाठ_अभिव्यक्ती*> newAssignRhsps(Ast_गाठ_अभिव्यक्ती* nodep, Ast_गाठ_DType* dtypep) {
        स्थिरांक std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_गाठ_अभिव्यक्ती*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        जर (AstCReset* स्थिरांक cresetp = VN_CAST(nodep, CReset)) {
            साठी (स्थिरांक Component& comp : compsr) {
                AstCReset* स्थिरांक newp = cresetp->clone_वृक्ष(असत्य);
                newp->dtypep(comp.dtypep);
                valueps.ढकला_back(newp);
            }
            परत valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        जर (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            पूर्णांक lsb = 0;  // LSB of the current term
            साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक termp : concatTerms(nodep)) {
                स्थिरांक पूर्णांक msb = lsb + termp->width() - 1;
                स्थिरांक माप_प्रकार first = componentIndex(dtypep, lsb);
                साठी (माप_प्रकार i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    स्थिरांक Component& comp = compsr[i];
                    स्थिरांक पूर्णांक partLsb = std::max(lsb, comp.lsb);
                    स्थिरांक पूर्णांक partMsb = std::min(msb, comp.msb);
                    File_ओळ* स्थिरांक flp = termp->fileline();
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? नवीन AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            परत valueps;
        }

        // Otherwise unpacked, so select each component
        साठी (माप_प्रकार i = 0; i < compsr.size(); ++i) {
            valueps.ढकला_back(newSelect(nodep->clone_वृक्ष(असत्य), dtypep, i));
        }
        परत valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_गाठ_अभिव्यक्ती* newBits(File_ओळ* flp, स्थिरांक Place* placep, पूर्णांक lsb, पूर्णांक msb) {
        Ast_गाठ_DType* स्थिरांक dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        जर (!placep->split) {
            AstVarRef* स्थिरांक refp = newRef(flp, placep, VAccess::READ);
            जर (lsb == 0 && msb == dtypep->width() - 1) परत refp;
            परत नवीन AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        स्थिरांक std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_गाठ_अभिव्यक्ती* resultp = nullptr;
        स्थिरांक माप_प्रकार first = componentIndex(dtypep, lsb);
        साठी (माप_प्रकार i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            स्थिरांक Component& comp = comps[i];
            स्थिरांक पूर्णांक partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            स्थिरांक पूर्णांक partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* स्थिरांक childp = placep->childrenp.at(i).get();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? नवीन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        परत resultp;
    }

    स्थिर शून्य addNewAssign(Ast_गाठ_Assign* origp, Ast_गाठ_अभिव्यक्ती* lhsp, Ast_गाठ_अभिव्यक्ती* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    शून्य expandSS(Ast_गाठ_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_ओळ* स्थिरांक flp = origp->fileline();
        स्थिरांक std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        जर (isPacked(rPlacep->dtypep)) {
            साठी (माप_प्रकार i = 0; i < compsr.size(); ++i) {
                Place* स्थिरांक lChildp = lPlacep->childrenp.at(i).get();
                Ast_गाठ_अभिव्यक्ती* स्थिरांक rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } अन्यथा {
            साठी (माप_प्रकार i = 0; i < compsr.size(); ++i) {
                Place* स्थिरांक lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    शून्य expandSN(Ast_गाठ_Assign* origp, Place* lPlacep, Ast_गाठ_अभिव्यक्ती* rhsp) {
        स्थिरांक std::vector<Ast_गाठ_अभिव्यक्ती*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        साठी (माप_प्रकार i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    शून्य expandNS(Ast_गाठ_Assign* origp, Ast_गाठ_अभिव्यक्ती* lhsp, Place* rPlacep) {
        File_ओळ* स्थिरांक flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        जर (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            परत;
        }
        // Unpacked of the same shape, component-wise
        Ast_गाठ_DType* स्थिरांक dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        स्थिरांक तार्किक readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            स्थिरांक VNUser2InUse user2InUse;
            परत lhsp->exists([](स्थिरांक AstVarRef* refp) {
                AstVarScope* स्थिरांक vscp = refp->varScopep();
                जर (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                जर (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                परत vscp->user2() == 3;
            });
        }();
        जर (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* स्थिरांक scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* स्थिरांक vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, नवीन AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, नवीन AstVarRef{flp, vscp, VAccess::READ});
            परत;
        }
        स्थिरांक माप_प्रकार size = dtypeComponents(dtypep).size();
        साठी (माप_प्रकार i = 0; i < size; ++i) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक newLhsp = newSelect(lhsp->clone_वृक्ष(असत्य), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    शून्य expand(Ast_गाठ_Assign* origp,  //
                Place* lPlacep, Ast_गाठ_अभिव्यक्ती* lhsp,  //
                Place* rPlacep, Ast_गाठ_अभिव्यक्ती* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_ओळ* स्थिरांक flp = origp->fileline();
        // A Place not split is referenced
        जर (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        जर (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        जर (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } अन्यथा जर (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } अन्यथा जर (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } अन्यथा {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    तार्किक isSplitBetween(स्थिरांक Place* placep, पूर्णांक lsb, पूर्णांक msb) {
        पर्यंत (placep->split) {
            स्थिरांक माप_प्रकार idx = componentIndex(placep->dtypep, lsb);
            जर (idx != componentIndex(placep->dtypep, msb)) परत सत्य;
            स्थिरांक Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        परत असत्य;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    शून्य hoistTerms(Ast_गाठ_Assign* origp, स्थिरांक Place* lPlacep) {
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = origp->rhsp();
        स्थिरांक std::vector<Ast_गाठ_अभिव्यक्ती*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        स्थिरांक VNUser2InUse user2InUse;
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* स्थिरांक scopep = lPlacep->rootp->vscp->scopep();
        पूर्णांक lsb = 0;  // LSB of the current term
        साठी (Ast_गाठ_अभिव्यक्ती* स्थिरांक termp : termps) {
            स्थिरांक पूर्णांक tLsb = lsb;
            स्थिरांक पूर्णांक tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            जर (!termp->user2()) सुरूठेवा;
            // Cheap to select from
            जर (isCheap(termp)) {
                termp->user2(0);
                सुरूठेवा;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            जर (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                सुरूठेवा;
            }
            // Bits can be selected without evaluating it more than once
            जर (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                सुरूठेवा;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_ओळ* स्थिरांक flp = termp->fileline();
            AstVarScope* स्थिरांक vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(नवीन AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* स्थिरांक tmpRefp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(नवीन AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    स्थिर शून्य markComponents(स्थिरांक Place* placep) {
        साठी (स्थिरांक std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(सत्य);
            जर (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    शून्य rewriteAssignment(स्थिरांक Assignment& assignment) {
        Place* स्थिरांक lPlacep = assignment.lPlacep;
        Place* स्थिरांक rPlacep = assignment.rPlacep;
        स्थिरांक तार्किक lSplit = lPlacep && lPlacep->split;
        स्थिरांक तार्किक rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        जर (!lSplit && !rSplit) परत;

        Ast_गाठ_Assign* स्थिरांक origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        जर (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            स्थिरांक VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = origp->rhsp();
            जर (rhsp->exists([](स्थिरांक AstVarRef* refp) { परत refp->varScopep()->user2(); })) {
                File_ओळ* स्थिरांक flp = rhsp->fileline();
                AstScope* स्थिरांक scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* स्थिरांक vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(नवीन AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* स्थिरांक tmpRefp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(नवीन AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        जर (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = origp->lhsp()->unlinkFrBack();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        जर (lSplit) VL_DO_DANGLING(ढकला_Deletep(lhsp), lhsp);
        जर (rSplit) VL_DO_DANGLING(ढकला_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ढकला_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर AstActive* comboActive(AstScope* scopep) {
        जर (!scopep->user1p()) scopep->user1p(scopep->comboActivep(सत्य));
        परत VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    शून्य driveFromComponents(Place* placep, AstActive* activep) {
        जर (!placep->split) परत;
        AstVarScope* स्थिरांक vscp = placep->vscp;
        File_ओळ* स्थिरांक flp = vscp->fileline();
        साठी (माप_प्रकार i = 0; i < placep->childrenp.size(); ++i) {
            Place* स्थिरांक childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsRefp = नवीन AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(नवीन AstAlways{नवीन AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        साठी (स्थिरांक std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        साठी (स्थिरांक Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        साठी (Place* स्थिरांक placep : m_state.rootps) {
            जर (!placep->split) सुरूठेवा;
            स्थिरांक AstVarScope* स्थिरांक vscp = placep->vscp;
            // Only traced ones at this point
            जर (!(vscp->varp()->isTrace() && vscp->isTrace())) सुरूठेवा;
            AstActive* स्थिरांक activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

शून्य V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("decompose", 0, dump_वृक्ष_Either_Level() >= 3);
}
