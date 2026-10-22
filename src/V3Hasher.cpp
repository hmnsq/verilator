// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode hash computation
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

#ضمن "V3PchAstMT.h"

#ضمن "V3Hasher.h"

#ضمن <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

صنف HasherVisitor final : عام VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ثابت منطقي m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_كلي*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    فراغ guardRecursion(Ast_كلي* ثابت nodep, std::function<فراغ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        إذا (!m_cacheInUser4) {
            إذا (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                أرجع;
            }
            m_visited.سك(nodep);
        }
        f();
    }

    V3Hash hash_كلي_And_Iterate(Ast_كلي* nodep, منطقي hashDType, منطقي hashChildren,
                              std::function<فراغ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        إذا (m_cacheInUser4 && nodep->user4()) {
            أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وإلا {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            إذا (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            إذا (hashChildren) iterateChildrenConst(nodep);  // Children
            إذا (m_cacheInUser4) nodep->user4(m_hash.درج());
            أرجع m_hash;
        }
    }

    // VISITORS

    constexpr ساكن منطقي HASH_DTYPE = true;
    constexpr ساكن منطقي HASH_CHILDREN = true;

    // Each visitor below contributes to the hash any node-specific content
    // that is not dependent on either of the following, as these are
    // included by default by hashNode:
    // - Node type (as given by AstNode::type().hash())
    // - Node dtype (unless !hashDType)
    // - child nodes (unless !hashChildren)
    //
    // The hash must be stable, which means in particular it cannot rely on
    // pointer values, or any other value that might differ between separate
    // invocations of Verilator over the same design.
    //
    // Note there is a circularity problem where some child nodes can back
    // to their ancestral nodes via member pointers, which can lead to an
    // infinite traversal. To break this, nodes that are subject to such
    // referencing and represent code which can reasonably be assumed not to
    // be equivalent to any other code, are hashed either by name (e.g.:
    // AstNodeModule), or by unique identifier (e.g.: AstNodeUOrStructDType).

    //------------------------------------------------------------
    // AstNode - Warns to help find missing cases

    فراغ visit(Ast_كلي* nodep) override {
#إذا VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    فراغ visit(Ast_كلي_Array_DType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->هغو();
            m_hash += nodep->دام();
        });
    }
    فراغ visit(Ast_كلي_UOr_Struct_DType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_لمب();
        });
    }
    فراغ visit(AstParamTypeDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ visit(AstMemberDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->unique_لمب();
        });
    }
    فراغ visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    فراغ visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstDynArrayDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstBasicDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().هغو();
            m_hash += nodep->nrange().دام();
        });
    }
    فراغ visit(AstCDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstConstDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstClassRefDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    فراغ visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    فراغ visit(AstQueueDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ visit(AstRefDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    فراغ visit(AstStreamDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstVoidDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstEnumDType* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_لمب();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    فراغ visit(Ast_كلي_فرچ* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstSel* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    فراغ visit(AstConst* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->لمب().toHash();
        });
    }
    فراغ visit(AstNullCheck* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstCCast* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    فراغ visit(AstVarRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            إذا (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وإلا {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    فراغ visit(AstVarXRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    فراغ visit(AstMemberSel* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstFScanF* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    فراغ visit(AstSScanF* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    فراغ visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ visit(AstTriggered* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            لكل (ثابت uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    فراغ visit(Ast_كلي_Stmt* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(Ast_كلي_CCall* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ visit(AstCaseItem* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(Ast_كلي_FTask_Ref* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    فراغ visit(AstCMethodHard* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    فراغ visit(AstCAwait* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ visit(AstCLocalScope* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstCoverInc* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ visit(AstDisplay* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ visit(AstMonitorOff* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    فراغ visit(AstJumpGo* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, خطأ, []() {});
    }
    فراغ visit(AstTraceInc* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ visit(Ast_كلي_Cover_Or_Assert* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    فراغ visit(Ast_كلي_Range* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(Ast_كلي_Module* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(Ast_كلي_Pre_Sel* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstClassExtends* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstForeachHeader* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstDefParam* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstArg* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstParseRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    فراغ visit(AstSenItem* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    فراغ visit(Ast_Sen_بشيا* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstSFormatF* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    فراغ visit(AstElabDisplay* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ visit(Ast_كلي_Cons_Item* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ خطأ,
                                     [هذا, nodep, dtypep]() {
                                         ثابت uint32_t size = dtypep->elementsConst();
                                         لكل (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    فراغ visit(AstPragma* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    فراغ visit(AstAttrOf* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    فراغ visit(Ast_كلي_File* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                إذا (m_cacheInUser4) nodep->user4(V3Hash{1}.درج());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    فراغ visit(AstVar* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ visit(AstScope* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    فراغ visit(AstVarScope* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    فراغ visit(AstEnumItem* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstTypedef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstTypedefFwd* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstActive* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ visit(AstCell* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    فراغ visit(AstCellInline* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [هذا, nodep]() { m_hash += nodep->name(); });
    }
    فراغ visit(Ast_كلي_FTask* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                إذا (m_cacheInUser4) nodep->user4(V3Hash{1}.درج());
            });
        });
    }
    فراغ visit(AstModport* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstModportVarRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    فراغ visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    فراغ visit(Ast_كلي_Procedure* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstBegin* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ visit(AstFork* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    فراغ visit(AstPin* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_لمب();
        });
    }
    فراغ visit(AstText* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    فراغ visit(AstTextBlock* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    فراغ visit(AstCStmt* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstCStmtUser* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstCExpr* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstCExprUser* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ visit(AstWith* nodep) override {
        m_hash += hash_كلي_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عام:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_كلي* nodep)
        : m_cacheInUser4{true} {
        iterateConst(nodep);
    }
    صنف Uncached {};
    HasherVisitor(ثابت Ast_كلي* nodep, Uncached)
        : m_cacheInUser4{خطأ} {
        iterateConst(const_cast<Ast_كلي*>(nodep));
    }
    V3Hash finalHash() ثابت { أرجع m_hash; }
    ~HasherVisitor() override = افتراضي;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_كلي* nodep) ثابت {
    إذا (!nodep->user4()) HasherVisitor{nodep};
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_كلي* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_كلي* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    أرجع visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(ثابت Ast_كلي& كلي) {
    أرجع static_cast<size_t>(V3Hasher::uncachedHash(&كلي).درج());
}
