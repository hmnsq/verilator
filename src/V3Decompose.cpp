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

वर्ग Decompose_आधारः VL_NOT_FINAL {
रक्षित:
    // TYPES

    // A component (element or member) of an aggregate type
    संरचना Component final {
        Ast_ग्रन्थिः_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        पूर्णाङ्क declIdx;  // The declared index of the element, if an array
        पूर्णाङ्क lsb;  // LSB in the whole value, if packed
        पूर्णाङ्क msb;  // MSB in the whole value, if packed
    };

    संरचना Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    संरचना Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        पूर्णाङ्क lsb;  // The first bit covered of this end
        पूर्णाङ्क otherLsb;  // The first bit covered of the other end
        पूर्णाङ्क विस्तारः;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    संरचना Place final {
        Place* parentp = शून्यसूचकः;  // The Place this is a component of, nullptr if original
        Place* rootp = शून्यसूचकः;  // The original variable (root of the tree), root points to itself
        Ast_ग्रन्थिः_DType* dtypep = शून्यसूचकः;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = शून्यसूचकः;
        // The component Places created so far, by index, sized on the first one created
        std::सदिशम्<std::unique_ptr<Place>> childrenp;
        std::सदिशम्<Copy> copies;  // The copies covering it, until split
        तार्किक splittable = सत्य;  // Can be split (there is no reason not to)
        तार्किक इच्छति_विभज = असत्य;  // Should be split (if possible, do split)
        तार्किक विभज = असत्य;  // Place was split
        तार्किक queued = असत्य;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    संरचना Select final {
        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;  // The select
        Place* placep;  // The component
        पूर्णाङ्क lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    संरचना Assignment final {
        Ast_ग्रन्थिः_नियोजय* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

सार्वजनिक:
    // The state of the pass, shared by the steps
    वर्ग State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ध्रुव VNUser4_अन्तर्_Use m_user4_अन्तर्_Use;

    सार्वजनिक:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ग्रन्थिः_DType, std::सदिशम्<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::सदिशम्<Place*> rootps;  // The original Places, in creation order
        std::सदिशम्<std::सदिशम्<Select>> chains;  // Select chains addressing components
        std::सदिशम्<Assignment> assignments;  // Assignments to expand if a side is split
    };

रक्षित:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    स्पष्ट Decompose_आधारः(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    स्थिर तार्किक isUnpacked(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (VN_IS(dtypep, UnpackArrayDType)) प्रत्यागम सत्य;
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        प्रत्यागम structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    स्थिर तार्किक isPacked(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        यदि (ध्रुव AstPackArrayDType* ध्रुव arrayp = VN_CAST(dtypep, PackArrayDType)) {
            प्रत्यागम arrayp->subDTypep()->विस्तारः() > 1;
        }
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep, StructDType);
        प्रत्यागम structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    स्थिर तार्किक isSplittableType(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        यदि (isPacked(dtypep)) प्रत्यागम v3Global.opt.fDecomposePacked();
        यदि (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            यदि (ध्रुव AstStructDType* ध्रुव structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                प्रति (ध्रुव AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    यदि (mp->valuep()) प्रत्यागम असत्य;
                }
            }
            प्रत्यागम v3Global.opt.fDecomposeUnpacked();
        }
        प्रत्यागम असत्य;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    स्थिर Ast_ग्रन्थिः_DType* dtype_सम्बन्धि(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) प्रत्यागम refp->varp()->dtypep();
        प्रत्यागम nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ध्रुव std::सदिशम्<Component>& dtypeComponents(Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::सदिशम्<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        यदि (compsr.रिक्तानि()) {
            यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव arrayp = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                ध्रुव तार्किक packed = isPacked(dtypep);
                ध्रुव VNumRange range = arrayp->declRange();
                ध्रुव तार्किक rev = packed && range.ascending();  // Only for naming components
                Ast_ग्रन्थिः_DType* ध्रुव subp = arrayp->subDTypep();
                प्रति (पूर्णाङ्क i = 0; i < range.elements(); ++i) {
                    ध्रुव पूर्णाङ्क lsb = packed ? i * subp->विस्तारः() : 0;
                    ध्रुव पूर्णाङ्क msb = packed ? lsb + subp->विस्तारः() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ध्रुव पूर्णाङ्क declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.प्रक्षिप_पृष्ठभागः({subp, शून्यसूचकः, declIdx, lsb, msb});
                }
            } अन्यथा {
                ध्रुव AstStructDType* ध्रुव structp = VN_AS(dtypep, StructDType);
                ध्रुव तार्किक packed = isPacked(dtypep);
                प्रति (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ध्रुव पूर्णाङ्क lsb = packed ? mp->lsb() : 0;
                    ध्रुव पूर्णाङ्क msb = packed ? lsb + mp->विस्तारः() - 1 : 0;
                    compsr.प्रक्षिप_पृष्ठभागः({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.अन्तः_2());
            }
        }

        // The components of this data type
        प्रत्यागम compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    आकार_t component_सूचकांक(Ast_ग्रन्थिः_DType* dtypep, पूर्णाङ्क bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ध्रुव std::सदिशम्<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ध्रुव स्वचालित it = std::अधस्तनम्_bound(compsr.begin(), compsr.अन्तः_2(), bit,
                                         [](ध्रुव Component& comp, पूर्णाङ्क b) {  //
                                             प्रत्यागम b > comp.msb;
                                         });
        प्रत्यागम it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* component_सम्बन्धि(Place* placep, आकार_t idx) {
        ध्रुव std::सदिशम्<Component>& comps = dtypeComponents(placep->dtypep);
        यदि (placep->childrenp.रिक्तानि()) placep->childrenp.resize(comps.आकार());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        यदि (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->इच्छति_विभज = placep->rootp->vscp->varp()->attr_विभज_Var();
        }
        प्रत्यागम childpr.get();
    }

    // Is 'nodep' cheap to clone
    स्थिर तार्किक isCheap(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Constants are cheap
        यदि (VN_IS(nodep, Const)) प्रत्यागम सत्य;
        // So are resets
        यदि (VN_IS(nodep, CReset)) प्रत्यागम सत्य;
        // Variable references are cheap for the most part
        यदि (ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            यदि (refp->varp()->isForced()) प्रत्यागम असत्य;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            यदि (refp->varp()->isSc()) प्रत्यागम असत्य;
            // Otherwise the path rooted here is cheap
            प्रत्यागम सत्य;
        }
        // Bit selects only with a constant LSB
        यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            प्रत्यागम VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        यदि (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(nodep, ArraySel)) {
            प्रत्यागम isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(nodep, StructSel)) {  //
            प्रत्यागम isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        यदि (ध्रुव AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {  //
            प्रत्यागम isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        प्रत्यागम असत्य;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

वर्ग Decompose_लेखः final : सार्वजनिक VNVisitorConst, सार्वजनिक Decompose_आधारः {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ग्रन्थिः_नियोजय* m_assignp = शून्यसूचकः;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    स्थिर शून्य warn_No_विभज(AstVar* varp, ध्रुव Ast_ग्रन्थिः* wherep, ध्रुव वर्ण* reasonp) {
        // Only warn if user requested splitting
        यदि (!varp->attr_विभज_Var()) प्रत्यागम;
        // A packed variable might still be split by V3BitBlast, which warns if not
        यदि (isPacked(varp->dtypep())) प्रत्यागम;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_संहिता::SPLITVAR, सत्य);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    स्थिर तार्किक isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        यदि (!varp->user1()) {
            ध्रुव तार्किक eligible = [&]() {
                // Check data type
                यदि (!isSplittableType(varp->dtypep())) प्रत्यागम असत्य;
                // Check properties
                ध्रुव वर्ण* ध्रुव reasonp = varp->cannot_विभज_विधा_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                यदि (reasonp) warn_No_विभज(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                प्रत्यागम !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        प्रत्यागम varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* place_सम्बन्धि(ध्रुव AstVarRef* refp) {
        AstVarScope* ध्रुव vscp = refp->varScopep();
        यदि (!isEligible(vscp->varp())) प्रत्यागम शून्यसूचकः;
        Place& place = m_state.places(vscp);
        यदि (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.इच्छति_विभज = vscp->varp()->attr_विभज_Var();
            m_state.rootps.प्रक्षिप_पृष्ठभागः(&place);
        }
        प्रत्यागम &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    स्थिर शून्य खण्डम्(Place* placep, ध्रुव Ast_ग्रन्थिः* wherep) {
        placep->splittable = असत्य;
        // Warn on the variable only, a component is split as far as possible, as requested
        यदि (!placep->parentp) {
            warn_No_विभज(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, std::सदिशम्<Select>& chain) {
        // Array selects correspond one to one with a component select
        यदि (AstArraySel* ध्रुव selp = VN_CAST(exprp, ArraySel)) {
            यदि (Place* ध्रुव fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ध्रुव AstConst* ध्रुव bitp = VN_CAST(selp->bitp(), Const);
                यदि (bitp && bitp->गन्तव्यम्_UQuad() < dtypeComponents(fromp->dtypep).आकार()) {
                    Place* ध्रुव childp = component_सम्बन्धि(fromp, bitp->गन्तव्यम्_UQuad());
                    fromp->इच्छति_विभज = सत्य;
                    chain.प्रक्षिप_पृष्ठभागः({selp, childp, 0});
                    प्रत्यागम childp;
                }
                // Otherwise block splitting of the Place
                खण्डम्(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            प्रत्यागम शून्यसूचकः;
        }
        // Struct selects correspond one to one with a component select
        यदि (AstStructSel* ध्रुव selp = VN_CAST(exprp, StructSel)) {
            यदि (Place* ध्रुव fromp = resolveChain(selp->fromp(), chain)) {
                यदि (!isUnpacked(fromp->dtypep)) प्रत्यागम शून्यसूचकः;
                AstStructDType* ध्रुव structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                यदि (!structp->user1_समुच्चयः_Once()) {
                    ध्रुव std::सदिशम्<Component>& compsr = dtypeComponents(structp);
                    प्रति (आकार_t i = 0; i < compsr.आकार(); ++i) compsr[i].memberp->user1(i);
                }
                ध्रुव Ast_ग्रन्थिः* ध्रुव memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ध्रुव childp = component_सम्बन्धि(fromp, memberp->user1());
                fromp->इच्छति_विभज = सत्य;
                chain.प्रक्षिप_पृष्ठभागः({selp, childp, 0});
                प्रत्यागम childp;
            }
            प्रत्यागम शून्यसूचकः;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        यदि (AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            यदि (!placep) {
                iterateConst(selp->lsbp());
                प्रत्यागम शून्यसूचकः;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ध्रुव AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const);
            यदि (!lsbp || lsbp->गन्तव्यम्_SInt() + selp->विस्तारः_Const() > placep->dtypep->विस्तारः()) {
                खण्डम्(placep, selp->fromp());
                iterateConst(selp->lsbp());
                प्रत्यागम शून्यसूचकः;
            }
            // Descend into the Place containing the bits, until exact
            पूर्णाङ्क lsb = lsbp->गन्तव्यम्_SInt();
            पूर्णाङ्क msb = lsb + selp->विस्तारः_Const() - 1;
            यावत् (lsb != 0 || msb != placep->dtypep->विस्तारः() - 1) {
                // Within a component of a type not split
                यदि (!isPacked(placep->dtypep)) प्रत्यागम शून्यसूचकः;
                ध्रुव आकार_t idx = component_सूचकांक(placep->dtypep, lsb);
                // Crossing components prevents splitting
                यदि (idx != component_सूचकांक(placep->dtypep, msb)) {
                    खण्डम्(placep, selp->fromp());
                    प्रत्यागम शून्यसूचकः;
                }
                placep->इच्छति_विभज = सत्य;
                ध्रुव Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_सम्बन्धि(placep, idx);
                chain.प्रक्षिप_पृष्ठभागः({selp, placep, lsb});
            }
            प्रत्यागम placep;
        }
        // Base case
        यदि (AstVarRef* ध्रुव refp = VN_CAST(exprp, VarRef)) प्रत्यागम place_सम्बन्धि(refp);
        // Not a select chain
        iterateConst(exprp);
        प्रत्यागम शून्यसूचकः;
    }

    // 'nodep' references the Place 'placep' whole
    शून्य referenced_सम्पूर्णम्(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        यदि (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            प्रत्यागम;
        }

        // Otherwise must block splitting of this Place
        खण्डम्(placep, nodep);
    }

    // Visit a select expression
    शून्य दर्शय_Select(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::सदिशम्<Select> chain;
        Place* ध्रुव placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        यदि (placep) referenced_सम्पूर्णम्(nodep, placep);
        // Record the chain for the rewriting phase
        यदि (!chain.रिक्तानि()) m_state.chains.प्रक्षिप_पृष्ठभागः(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    स्थिर तार्किक hasReversedRange(ध्रुव Ast_ग्रन्थिः_DType* aDTypep, ध्रुव Ast_ग्रन्थिः_DType* bDTypep) {
        ध्रुव AstUnpackArrayDType* ध्रुव aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ध्रुव AstUnpackArrayDType* ध्रुव bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        यदि (!aArrayp || !bArrayp) प्रत्यागम असत्य;
        यदि (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) प्रत्यागम सत्य;
        प्रत्यागम hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    शून्य दर्शय_Assignment(Ast_ग्रन्थिः_नियोजय* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ध्रुव तार्किक reversed = hasReversedRange(dtype_सम्बन्धि(nodep->lhsp()), dtype_सम्बन्धि(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? शून्यसूचकः : nodep;
        iterateChildrenConst(nodep);
        यदि (!m_assignp) प्रत्यागम;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp();
        Place* ध्रुव lPlacep = lhsp->user1u().गन्तव्यम्<Place*>();
        Place* ध्रुव rPlacep = rhsp->user1u().गन्तव्यम्<Place*>();

        // Is it a splittable assignment
        ध्रुव तार्किक splittable = [&]() {
            // Assignment between two Places is splittable
            यदि (lPlacep && rPlacep) प्रत्यागम सत्य;
            // If the LHS is a Place, the RHS must be a splittable expression
            यदि (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                यदि (isPacked(lPlacep->dtypep)) प्रत्यागम सत्य;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                प्रत्यागम isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            यदि (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                यदि (isPacked(rPlacep->dtypep)) प्रत्यागम सत्य;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                प्रत्यागम isCheap(lhsp);
            }
            // Otherwise not splittable
            प्रत्यागम असत्य;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        यदि (!splittable) {
            यदि (rPlacep) खण्डम्(rPlacep, rhsp);
            यदि (lPlacep) खण्डम्(lPlacep, lhsp);
            प्रत्यागम;
        }

        // If both sides are places, record the whole copies for the Decision phase
        यदि (lPlacep && rPlacep) {
            ध्रुव पूर्णाङ्क विस्तारः = lPlacep->dtypep->विस्तारः();  // Unused when unpacked
            lPlacep->copies.प्रक्षिप_पृष्ठभागः({rPlacep, 0, 0, विस्तारः});
            rPlacep->copies.प्रक्षिप_पृष्ठभागः({lPlacep, 0, 0, विस्तारः});
        }
        // Record for the rewriting phase
        m_state.assignments.प्रक्षिप_पृष्ठभागः({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    शून्य दर्शय(Ast_ग्रन्थिः_DType*) override {}  // No references in data types
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    शून्य दर्शय(AstArraySel* nodep) override { दर्शय_Select(nodep); }
    शून्य दर्शय(AstStructSel* nodep) override { दर्शय_Select(nodep); }
    शून्य दर्शय(AstSel* nodep) override { दर्शय_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    शून्य दर्शय(Ast_नियोजय* nodep) override { दर्शय_Assignment(nodep); }
    शून्य दर्शय(Ast_नियोजय_W* nodep) override { दर्शय_Assignment(nodep); }
    शून्य दर्शय(Ast_नियोजय_Dly* nodep) override { दर्शय_Assignment(nodep); }
    // VISITORS - References to Places
    शून्य दर्शय(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        यदि (Place* ध्रुव placep = place_सम्बन्धि(nodep)) referenced_सम्पूर्णम्(nodep, placep);
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ध्रुव varp = nodep->varp();
        यदि (!isEligible(varp)) प्रत्यागम;
        varp->user1(2);  // Mark not eligible
        warn_No_विभज(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_लेखः(State& state, AstNetlist* netlistp)
        : Decompose_आधारः{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        प्रति (Place* ध्रुव placep : m_state.rootps) {
            यदि (!isEligible(placep->vscp->varp())) placep->splittable = असत्य;
        }
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state, AstNetlist* netlistp) { Decompose_लेखः{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

वर्ग DecomposeDecision final : सार्वजनिक Decompose_आधारः {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE
    std::सदिशम्<Place*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::सदिशम्<AstVar*>> m_विभज_Varps;
    VDouble0 m_stat_विभज_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_विभज_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_विभज_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_विभज_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    स्थिर तार्किक can_विभज(ध्रुव Place* placep) {
        // Not if not splittable
        यदि (!placep->splittable) प्रत्यागम असत्य;
        // Not if already split
        यदि (placep->विभज) प्रत्यागम असत्य;
        // Original variable or its parent was already split
        प्रत्यागम !placep->parentp || placep->parentp->विभज;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    शून्य पङ्क्तौ_निक्षिप(Place* placep) {
        यदि (placep->queued || !placep->इच्छति_विभज || !can_विभज(placep)) प्रत्यागम;
        placep->queued = सत्य;
        m_worklist.प्रक्षिप_पृष्ठभागः(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    शून्य createComponents(Place* placep) {
        AstVarScope* ध्रुव vscp = placep->vscp;
        AstVar* ध्रुव varp = vscp->varp();
        std::सदिशम्<AstVar*>& varps = m_विभज_Varps(varp);
        // Create the component AstVars on first encounter
        यदि (varps.रिक्तानि()) {
            ध्रुव तार्किक packed = isPacked(varp->dtypep());
            यदि (placep->parentp) {
                ++(packed ? m_stat_विभज_Packed_Comp : m_stat_विभज_Unpacked_Comp);
            } अन्यथा {
                ++(packed ? m_stat_विभज_Packed_Orig : m_stat_विभज_Unpacked_Orig);
            }
            File_पङ्क्तिः* ध्रुव flp = varp->fileline();
            ध्रुव VVarType varType = varp->varType();
            ध्रुव std::string name = varp->name();
            प्रति (ध्रुव Component& comp : dtypeComponents(varp->dtypep())) {
                ध्रुव std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(comp.declIdx) + "__KET__";
                AstVar* ध्रुव newp = नूतन AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_विभज_Attr_From(varp);
                varps.प्रक्षिप_पृष्ठभागः(newp);
                varp->सङ्कलय_अत्र_This_As_अग्र(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ध्रुव scopep = vscp->scopep();
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
        प्रति (आकार_t i = 0; i < varps.आकार(); ++i) {
            AstVarScope* ध्रुव newp = नूतन AstVarScope{flp, scopep, varps[i]};
            component_सम्बन्धि(placep, i)->vscp = newp;
            vscp->सङ्कलय_अत्र_This_As_अग्र(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    शून्य सङ्कलय_Copy(Place* placep, ध्रुव Copy& copy) {
        UASSERT(!placep->विभज, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        यदि (!placep->splittable) प्रत्यागम;
        // Add the copy to the Place
        placep->copies.प्रक्षिप_पृष्ठभागः(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        यदि (!isPacked(placep->dtypep)) प्रत्यागम;
        यदि (copy.lsb != 0 || copy.विस्तारः != placep->dtypep->विस्तारः()) {
            placep->इच्छति_विभज = सत्य;
            पङ्क्तौ_निक्षिप(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    शून्य newCopy(Place* ap, पूर्णाङ्क aLsb, Place* bp, पूर्णाङ्क bLsb, पूर्णाङ्क विस्तारः) {
        UASSERT(ap != bp, "Copy within a Place");
        सङ्कलय_Copy(ap, {bp, aLsb, bLsb, विस्तारः});
        सङ्कलय_Copy(bp, {ap, bLsb, aLsb, विस्तारः});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    शून्य विभज_Copy(Place* placep, ध्रुव Copy& copy) {
        Place* ध्रुव otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        यदि (isPacked(placep->dtypep)) {
            ध्रुव std::सदिशम्<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ध्रुव पूर्णाङ्क lsb = copy.lsb;
            ध्रुव पूर्णाङ्क msb = copy.lsb + copy.विस्तारः - 1;
            ध्रुव आकार_t first = component_सूचकांक(placep->dtypep, lsb);
            प्रति (आकार_t i = first; i < comps.आकार() && comps[i].lsb <= msb; ++i) {
                ध्रुव Component& comp = comps[i];
                ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                newCopy(component_सम्बन्धि(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            प्रत्यागम;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->इच्छति_विभज = सत्य;
        पङ्क्तौ_निक्षिप(otherp);
        // The sides are the same shape, so pairwise whole copies
        ध्रुव आकार_t आकार = dtypeComponents(placep->dtypep).आकार();
        प्रति (आकार_t i = 0; i < आकार; ++i) {
            Place* ध्रुव ap = component_सम्बन्धि(placep, i);
            यदि (!ap->splittable) अनुवर्त;
            Place* ध्रुव bp = component_सम्बन्धि(otherp, i);
            यदि (!bp->splittable) अनुवर्त;
            newCopy(ap, 0, bp, 0, ap->dtypep->विस्तारः());
        }
    }

    // Split the Place
    शून्य विभज_Place(Place* placep) {
        UASSERT(!placep->विभज, "Place split twice");
        // It is now split
        placep->विभज = सत्य;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ध्रुव आकार_t आकार = dtypeComponents(placep->dtypep).आकार();
        प्रति (आकार_t i = 0; i < आकार; ++i) पङ्क्तौ_निक्षिप(component_सम्बन्धि(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        प्रति (ध्रुव Copy& copy : placep->copies) {
            यदि (!copy.otherp->विभज) विभज_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    स्पष्ट DecomposeDecision(State& state)
        : Decompose_आधारः{state} {
        // Enqueue the original variables
        प्रति (Place* ध्रुव placep : m_state.rootps) पङ्क्तौ_निक्षिप(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        यावत् (!m_worklist.रिक्तानि()) {
            Place* ध्रुव placep = m_worklist.पृष्ठभागः();
            m_worklist.निष्कासय_पृष्ठभागः();
            placep->queued = असत्य;
            विभज_Place(placep);
        }

        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_विभज_Unpacked_Orig);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_विभज_Packed_Orig);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_विभज_Unpacked_Comp);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_विभज_Packed_Comp);
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

वर्ग DecomposeRewrite final : सार्वजनिक VNDeleter, सार्वजनिक Decompose_आधारः {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ध्रुव VNUser1_अन्तर्_Use m_user1_अन्तर्_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    स्थिर AstVarRef* newRef(File_पङ्क्तिः* flp, ध्रुव Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        प्रत्यागम नूतन AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    शून्य rewriteChain(ध्रुव std::सदिशम्<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ध्रुव Select* selectp = शून्यसूचकः;
        प्रति (ध्रुव Select& select : chain) {
            यदि (select.placep->parentp->विभज) selectp = &select;
        }
        यदि (!selectp) प्रत्यागम;
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ध्रुव VAccess access = [&]() {
            ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* rootp = chain.front().exprp;
            यावत् (सत्य) {
                यदि (ध्रुव AstArraySel* ध्रुव selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } अन्यथा यदि (ध्रुव AstStructSel* ध्रुव selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } अन्यथा यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } अन्यथा {
                    प्रत्यागम VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_पङ्क्तिः* ध्रुव flp = exprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = newRef(flp, selectp->placep, access);
        यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(exprp, Sel)) {
            ध्रुव पूर्णाङ्क विस्तारः = selp->विस्तारः_Const();
            यदि (विस्तारः != newp->विस्तारः()) newp = नूतन AstSel{flp, newp, selectp->lsb, विस्तारः};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ग्रन्थिः_व्यञ्जकम्* newSelect(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, Ast_ग्रन्थिः_DType* dtypep, आकार_t idx) {
        File_पङ्क्तिः* ध्रुव flp = fromp->fileline();
        ध्रुव Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        यदि (isPacked(dtypep)) {
            AstSel* ध्रुव selp = नूतन AstSel{flp, fromp, comp.lsb, comp.dtypep->विस्तारः()};
            selp->dtypep(comp.dtypep);
            प्रत्यागम selp;
        }
        // If unpacked array, it's an ArraySel
        यदि (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ध्रुव पूर्णाङ्क i = static_cast<पूर्णाङ्क>(idx);
            UASSERT_OBJ(static_cast<आकार_t>(i) == idx, fromp, "Index overflow");
            प्रत्यागम नूतन AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ध्रुव selp = नूतन AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        प्रत्यागम selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    स्थिर std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> concatTerms(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> termps;
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> राशिः{nodep};
        यावत् (!राशिः.रिक्तानि()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = राशिः.पृष्ठभागः();
            राशिः.निष्कासय_पृष्ठभागः();
            यदि (AstConcat* ध्रुव concatp = VN_CAST(exprp, Concat)) {
                राशिः.प्रक्षिप_पृष्ठभागः(concatp->lhsp());
                राशिः.प्रक्षिप_पृष्ठभागः(concatp->rhsp());
                अनुवर्त;
            }
            यदि (AstReplicate* ध्रुव repp = VN_CAST(exprp, Replicate)) {
                ध्रुव AstConst* ध्रुव countp = VN_AS(repp->countp(), Const);
                प्रति (uint32_t i = 0; i < countp->गन्तव्यम्_UInt(); ++i) राशिः.प्रक्षिप_पृष्ठभागः(repp->srcp());
                अनुवर्त;
            }
            termps.प्रक्षिप_पृष्ठभागः(exprp);
        }
        प्रत्यागम termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    स्थिर तार्किक isSliceable(ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (ध्रुव AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            प्रत्यागम VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        यदि (ध्रुव AstConcat* ध्रुव catp = VN_CAST(nodep, Concat)) {
            प्रत्यागम isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        यदि (ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            प्रत्यागम isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        यदि (ध्रुव AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
            प्रत्यागम isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        यदि (ध्रुव AstOr* ध्रुव orp = VN_CAST(nodep, Or)) {
            प्रत्यागम isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        यदि (ध्रुव AstXor* ध्रुव xorp = VN_CAST(nodep, Xor)) {
            प्रत्यागम isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        यदि (ध्रुव AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {
            प्रत्यागम isSliceable(extp->lhsp());
        }
        यदि (ध्रुव AstNot* ध्रुव notp = VN_CAST(nodep, Not)) {  //
            प्रत्यागम isSliceable(notp->lhsp());
        }
        यदि (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep, Replicate)) {
            प्रत्यागम isSliceable(repp->srcp());
        }
        प्रत्यागम isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* new_खण्डिका(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, पूर्णाङ्क lsb, पूर्णाङ्क विस्तारः) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        // A constant, as a new constant of the bits
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
            V3_सङ्ख्या अङ्कः{nodep, विस्तारः};
            अङ्कः.opSel(constp->अङ्कः(), lsb + विस्तारः - 1, lsb);
            प्रत्यागम नूतन AstConst{flp, अङ्कः};
        }
        // A constant bit select, from the bits of what it selects from
        यदि (AstSel* ध्रुव selp = VN_CAST(nodep, Sel)) {
            यदि (ध्रुव AstConst* ध्रुव lsbp = VN_CAST(selp->lsbp(), Const)) {
                प्रत्यागम new_खण्डिका(selp->fromp(), lsbp->गन्तव्यम्_SInt() + lsb, विस्तारः);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        यदि (AstConcat* ध्रुव catp = VN_CAST(nodep, Concat)) {
            ध्रुव पूर्णाङ्क r_विस्तारः = catp->rhsp()->विस्तारः();
            ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
            यदि (msb < r_विस्तारः) प्रत्यागम new_खण्डिका(catp->rhsp(), lsb, विस्तारः);
            यदि (lsb >= r_विस्तारः) प्रत्यागम new_खण्डिका(catp->lhsp(), lsb - r_विस्तारः, विस्तारः);
            प्रत्यागम नूतन AstConcat{flp, new_खण्डिका(catp->lhsp(), 0, msb - r_विस्तारः + 1),
                                 new_खण्डिका(catp->rhsp(), lsb, r_विस्तारः - lsb)};
        }
        यदि (AstCond* ध्रुव condp = VN_CAST(nodep, Cond)) {
            प्रत्यागम नूतन AstCond{flp, condp->condp()->clone_वृक्षः_विशुद्धम्(असत्य),
                               new_खण्डिका(condp->thenp(), lsb, विस्तारः),
                               new_खण्डिका(condp->elsep(), lsb, विस्तारः)};
        }
        यदि (AstAnd* ध्रुव andp = VN_CAST(nodep, And)) {
            प्रत्यागम नूतन AstAnd{flp, new_खण्डिका(andp->lhsp(), lsb, विस्तारः),
                              new_खण्डिका(andp->rhsp(), lsb, विस्तारः)};
        }
        यदि (AstOr* ध्रुव orp = VN_CAST(nodep, Or)) {
            प्रत्यागम नूतन AstOr{flp, new_खण्डिका(orp->lhsp(), lsb, विस्तारः),
                             new_खण्डिका(orp->rhsp(), lsb, विस्तारः)};
        }
        यदि (AstXor* ध्रुव xorp = VN_CAST(nodep, Xor)) {
            प्रत्यागम नूतन AstXor{flp, new_खण्डिका(xorp->lhsp(), lsb, विस्तारः),
                              new_खण्डिका(xorp->rhsp(), lsb, विस्तारः)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        यदि (AstExtend* ध्रुव extp = VN_CAST(nodep, Extend)) {
            ध्रुव पूर्णाङ्क src_विस्तारः = extp->lhsp()->विस्तारः();
            ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
            यदि (msb < src_विस्तारः) प्रत्यागम new_खण्डिका(extp->lhsp(), lsb, विस्तारः);
            यदि (lsb >= src_विस्तारः) प्रत्यागम नूतन AstConst{flp, AstConst::Widthed_मानम्{}, विस्तारः, 0};
            प्रत्यागम नूतन AstConcat{
                flp, नूतन AstConst{flp, AstConst::Widthed_मानम्{}, msb - src_विस्तारः + 1, 0},
                new_खण्डिका(extp->lhsp(), lsb, src_विस्तारः - lsb)};
        }
        यदि (AstNot* ध्रुव notp = VN_CAST(nodep, Not)) {
            प्रत्यागम नूतन AstNot{flp, new_खण्डिका(notp->lhsp(), lsb, विस्तारः)};
        }
        // A replication, from the portions of the copies overlapping the bits
        यदि (AstReplicate* ध्रुव repp = VN_CAST(nodep, Replicate)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव srcp = repp->srcp();
            ध्रुव पूर्णाङ्क src_विस्तारः = srcp->विस्तारः();
            ध्रुव पूर्णाङ्क msb = lsb + विस्तारः - 1;
            Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
            प्रति (पूर्णाङ्क copyLsb = lsb / src_विस्तारः * src_विस्तारः; copyLsb <= msb; copyLsb += src_विस्तारः) {
                ध्रुव पूर्णाङ्क partLsb = std::max(lsb, copyLsb) - copyLsb;
                ध्रुव पूर्णाङ्क partMsb = std::min(msb, copyLsb + src_विस्तारः - 1) - copyLsb;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitsp = new_खण्डिका(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
            }
            प्रत्यागम resultp;
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव clonep = nodep->clone_वृक्षः_विशुद्धम्(असत्य);
        यदि (lsb == 0 && विस्तारः == nodep->विस्तारः()) प्रत्यागम clonep;
        प्रत्यागम नूतन AstSel{flp, clonep, lsb, विस्तारः};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> new_नियोजय_Rhsps(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव std::सदिशम्<Component>& compsr = dtypeComponents(dtypep);
        std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> valueps;
        valueps.reserve(compsr.आकार());

        // If CReset, it is duplicated for each component
        यदि (AstCReset* ध्रुव cresetp = VN_CAST(nodep, CReset)) {
            प्रति (ध्रुव Component& comp : compsr) {
                AstCReset* ध्रुव newp = cresetp->clone_वृक्षः(असत्य);
                newp->dtypep(comp.dtypep);
                valueps.प्रक्षिप_पृष्ठभागः(newp);
            }
            प्रत्यागम valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        यदि (isPacked(dtypep)) {
            valueps.resize(compsr.आकार(), शून्यसूचकः);
            पूर्णाङ्क lsb = 0;  // LSB of the current term
            प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : concatTerms(nodep)) {
                ध्रुव पूर्णाङ्क msb = lsb + termp->विस्तारः() - 1;
                ध्रुव आकार_t first = component_सूचकांक(dtypep, lsb);
                प्रति (आकार_t i = first; i < compsr.आकार() && compsr[i].lsb <= msb; ++i) {
                    ध्रुव Component& comp = compsr[i];
                    ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb);
                    ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb);
                    File_पङ्क्तिः* ध्रुव flp = termp->fileline();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sp = new_खण्डिका(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? नूतन AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            प्रत्यागम valueps;
        }

        // Otherwise unpacked, so select each component
        प्रति (आकार_t i = 0; i < compsr.आकार(); ++i) {
            valueps.प्रक्षिप_पृष्ठभागः(newSelect(nodep->clone_वृक्षः(असत्य), dtypep, i));
        }
        प्रत्यागम valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ग्रन्थिः_व्यञ्जकम्* new_द्वयङ्काः(File_पङ्क्तिः* flp, ध्रुव Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->विभज, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        यदि (!placep->विभज) {
            AstVarRef* ध्रुव refp = newRef(flp, placep, VAccess::READ);
            यदि (lsb == 0 && msb == dtypep->विस्तारः() - 1) प्रत्यागम refp;
            प्रत्यागम नूतन AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ध्रुव std::सदिशम्<Component>& comps = dtypeComponents(dtypep);
        Ast_ग्रन्थिः_व्यञ्जकम्* resultp = शून्यसूचकः;
        ध्रुव आकार_t first = component_सूचकांक(dtypep, lsb);
        प्रति (आकार_t i = first; i < comps.आकार() && comps[i].lsb <= msb; ++i) {
            ध्रुव Component& comp = comps[i];
            ध्रुव पूर्णाङ्क partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ध्रुव पूर्णाङ्क partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ध्रुव childp = placep->childrenp.at(i).get();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव bitsp = new_द्वयङ्काः(flp, childp, partLsb, partMsb);
            resultp = resultp ? नूतन AstConcat{flp, bitsp, resultp} : bitsp;
        }
        प्रत्यागम resultp;
    }

    स्थिर शून्य सङ्कलय_New_नियोजय(Ast_ग्रन्थिः_नियोजय* origp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        origp->सङ्कलय_अत्र_This_As_अग्र(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    शून्य expandSS(Ast_ग्रन्थिः_नियोजय* origp, Place* lPlacep, Place* rPlacep) {
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        ध्रुव std::सदिशम्<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        यदि (isPacked(rPlacep->dtypep)) {
            प्रति (आकार_t i = 0; i < compsr.आकार(); ++i) {
                Place* ध्रुव lChildp = lPlacep->childrenp.at(i).get();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rp = new_द्वयङ्काः(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, शून्यसूचकः, शून्यसूचकः, rp);
            }
        } अन्यथा {
            प्रति (आकार_t i = 0; i < compsr.आकार(); ++i) {
                Place* ध्रुव lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, शून्यसूचकः, rPlacep->childrenp.at(i).get(), शून्यसूचकः);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    शून्य expandSN(Ast_ग्रन्थिः_नियोजय* origp, Place* lPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> valueps = new_नियोजय_Rhsps(rhsp, lPlacep->dtypep);
        प्रति (आकार_t i = 0; i < valueps.आकार(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), शून्यसूचकः, शून्यसूचकः, valueps[i]);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    शून्य expandNS(Ast_ग्रन्थिः_नियोजय* origp, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Place* rPlacep) {
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        यदि (isPacked(rPlacep->dtypep)) {
            सङ्कलय_New_नियोजय(origp, lhsp, new_द्वयङ्काः(flp, rPlacep, 0, rPlacep->dtypep->विस्तारः() - 1));
            प्रत्यागम;
        }
        // Unpacked of the same shape, component-wise
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = dtype_सम्बन्धि(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ध्रुव तार्किक readsWrittenVar = !VN_IS(origp, नियोजय_Dly) && [&]() {
            ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
            प्रत्यागम lhsp->exists([](ध्रुव AstVarRef* refp) {
                AstVarScope* ध्रुव vscp = refp->varScopep();
                यदि (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                यदि (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                प्रत्यागम vscp->user2() == 3;
            });
        }();
        यदि (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ध्रुव scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, शून्यसूचकः, नूतन AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, शून्यसूचकः);
            सङ्कलय_New_नियोजय(origp, lhsp, नूतन AstVarRef{flp, vscp, VAccess::READ});
            प्रत्यागम;
        }
        ध्रुव आकार_t आकार = dtypeComponents(dtypep).आकार();
        प्रति (आकार_t i = 0; i < आकार; ++i) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newLhsp = newSelect(lhsp->clone_वृक्षः(असत्य), dtypep, i);
            expand(origp, शून्यसूचकः, newLhsp, rPlacep->childrenp.at(i).get(), शून्यसूचकः);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    शून्य expand(Ast_ग्रन्थिः_नियोजय* origp,  //
                Place* lPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp,  //
                Place* rPlacep, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_पङ्क्तिः* ध्रुव flp = origp->fileline();
        // A Place not split is referenced
        यदि (lPlacep && !lPlacep->विभज) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = शून्यसूचकः;
        }
        यदि (rPlacep && !rPlacep->विभज) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = शून्यसूचकः;
        }
        यदि (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } अन्यथा यदि (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } अन्यथा यदि (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } अन्यथा {
            सङ्कलय_New_नियोजय(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    तार्किक is_विभज_Between(ध्रुव Place* placep, पूर्णाङ्क lsb, पूर्णाङ्क msb) {
        यावत् (placep->विभज) {
            ध्रुव आकार_t idx = component_सूचकांक(placep->dtypep, lsb);
            यदि (idx != component_सूचकांक(placep->dtypep, msb)) प्रत्यागम सत्य;
            ध्रुव Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        प्रत्यागम असत्य;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    शून्य hoistTerms(Ast_ग्रन्थिः_नियोजय* origp, ध्रुव Place* lPlacep) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp();
        ध्रुव std::सदिशम्<Ast_ग्रन्थिः_व्यञ्जकम्*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ध्रुव scopep = lPlacep->rootp->vscp->scopep();
        पूर्णाङ्क lsb = 0;  // LSB of the current term
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp : termps) {
            ध्रुव पूर्णाङ्क tLsb = lsb;
            ध्रुव पूर्णाङ्क tMsb = lsb + termp->विस्तारः() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            यदि (!termp->user2()) अनुवर्त;
            // Cheap to select from
            यदि (isCheap(termp)) {
                termp->user2(0);
                अनुवर्त;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            यदि (termp->is_विशुद्धम्() && termp->user2() == 1 && !is_विभज_Between(lPlacep, tLsb, tMsb)) {
                अनुवर्त;
            }
            // Bits can be selected without evaluating it more than once
            यदि (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                अनुवर्त;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_पङ्क्तिः* ध्रुव flp = termp->fileline();
            AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ध्रुव tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            origp->सङ्कलय_अत्र_This_As_अग्र(नूतन Ast_नियोजय{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    स्थिर शून्य markComponents(ध्रुव Place* placep) {
        प्रति (ध्रुव std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(सत्य);
            यदि (childp->विभज) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    शून्य rewriteAssignment(ध्रुव Assignment& assignment) {
        Place* ध्रुव lPlacep = assignment.lPlacep;
        Place* ध्रुव rPlacep = assignment.rPlacep;
        ध्रुव तार्किक l_विभज = lPlacep && lPlacep->विभज;
        ध्रुव तार्किक r_विभज = rPlacep && rPlacep->विभज;
        // Nothing to do if neither side is split
        यदि (!l_विभज && !r_विभज) प्रत्यागम;

        Ast_ग्रन्थिः_नियोजय* ध्रुव origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        यदि (l_विभज && !r_विभज && !VN_IS(origp, नियोजय_Dly)) {
            ध्रुव VNUser2_अन्तर्_Use user2_अन्तर्_Use;
            markComponents(lPlacep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp();
            यदि (rhsp->exists([](ध्रुव AstVarRef* refp) { प्रत्यागम refp->varScopep()->user2(); })) {
                File_पङ्क्तिः* ध्रुव flp = rhsp->fileline();
                AstScope* ध्रुव scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ध्रुव vscp = m_tmps.make(flp, scopep, dtype_सम्बन्धि(rhsp));
                rhsp->replaceWith(नूतन AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ध्रुव tmpRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
                origp->सङ्कलय_अत्र_This_As_अग्र(नूतन Ast_नियोजय{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        यदि (l_विभज && !r_विभज && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = origp->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = origp->rhsp()->unlink_Fr_पृष्ठभागः();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_विभज ? lPlacep : शून्यसूचकः, l_विभज ? शून्यसूचकः : lhsp,  //
               r_विभज ? rPlacep : शून्यसूचकः, r_विभज ? शून्यसूचकः : rhsp);
        यदि (l_विभज) VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
        यदि (r_विभज) VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(origp->unlink_Fr_पृष्ठभागः()), origp);
    }

    // The combinational AstActive of the scope, cached
    स्थिर Ast_सक्रिय* combo_सक्रिय(AstScope* scopep) {
        यदि (!scopep->user1p()) scopep->user1p(scopep->comboActivep(सत्य));
        प्रत्यागम VN_AS(scopep->user1p(), सक्रिय);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    शून्य driveFromComponents(Place* placep, Ast_सक्रिय* activep) {
        यदि (!placep->विभज) प्रत्यागम;
        AstVarScope* ध्रुव vscp = placep->vscp;
        File_पङ्क्तिः* ध्रुव flp = vscp->fileline();
        प्रति (आकार_t i = 0; i < placep->childrenp.आकार(); ++i) {
            Place* ध्रुव childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsRefp = नूतन AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->सङ्कलय_Stmtsp(नूतन AstAlways{नूतन Ast_नियोजय_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    स्पष्ट DecomposeRewrite(State& state)
        : Decompose_आधारः{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        प्रति (ध्रुव std::सदिशम्<Select>& chain : m_state.chains) rewriteChain(chain);
        प्रति (ध्रुव Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        प्रति (Place* ध्रुव placep : m_state.rootps) {
            यदि (!placep->विभज) अनुवर्त;
            ध्रुव AstVarScope* ध्रुव vscp = placep->vscp;
            // Only traced ones at this point
            यदि (!(vscp->varp()->isTrace() && vscp->isTrace())) अनुवर्त;
            Ast_सक्रिय* ध्रुव activep = combo_सक्रिय(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::सङ्कलय_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

सार्वजनिक:
    स्थिर शून्य apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

शून्य V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_आधारः::State state;
        Decompose_लेखः::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("decompose", 0, dump_वृक्षः_Either_Level() >= 3);
}
