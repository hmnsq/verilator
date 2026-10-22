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
    ثابت منطقي m_cache_اندريون_User4;  // Use user4 to cache each V3Hash?
    std::سيٽ<Ast_ڳنڍ*> m_گهميل;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    فراغ guardRecursion(Ast_ڳنڍ* ثابت nodep, std::function<فراغ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        إذا (!m_cache_اندريون_User4) {
            إذا (m_گهميل.ڳوليو(nodep) != m_گهميل.پڇاڙي()) {
                m_hash += V3Hash{nodep->name()};
                أرجع;
            }
            m_گهميل.وجهو(nodep);
        }
        f();
    }

    V3Hash hash_ڳنڍ_And_Iterate(Ast_ڳنڍ* nodep, منطقي hashDType, منطقي hashChildren,
                              std::function<فراغ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        إذا (m_cache_اندريون_User4 && nodep->user4()) {
            أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وإلا {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            إذا (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            إذا (hashChildren) iterateChildrenConst(nodep);  // Children
            إذا (m_cache_اندريون_User4) nodep->user4(m_hash.قدر());
            أرجع m_hash;
        }
    }

    // VISITORS

    constexpr ساكن منطقي HASH_DTYPE = سچ;
    constexpr ساكن منطقي HASH_CHILDREN = سچ;

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

    فراغ گهمو(Ast_ڳنڍ* nodep) override {
#إذا VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#جيڪڏهن_پورو
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    فراغ گهمو(Ast_ڳنڍ_Array_DType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->کاٻي();
            m_hash += nodep->ساڄي();
        });
    }
    فراغ گهمو(Ast_ڳنڍ_UOr_Struct_DType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_انگ();
        });
    }
    فراغ گهمو(AstParamTypeDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ گهمو(AstMemberDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstDefImplicitDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->unique_انگ();
        });
    }
    فراغ گهمو(AstAssocArrayDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    فراغ گهمو(AstBracketArrayDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstDynArrayDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstBasicDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().کاٻي();
            m_hash += nodep->nrange().ساڄي();
        });
    }
    فراغ گهمو(AstCDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstConstDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstClassRefDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    فراغ گهمو(AstIfaceRefDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    فراغ گهمو(Ast_قطار_DType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ گهمو(AstRefDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    فراغ گهمو(Ast_وهڪرو_DType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstVoidDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstEnumDType* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_انگ();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    فراغ گهمو(Ast_ڳنڍ_اظهار* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstSel* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->ويڪر_Const();
        });
    }
    فراغ گهمو(AstConst* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->انگ().منزل_Hash();
        });
    }
    فراغ گهمو(AstNullCheck* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstCCast* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->سائيز();
        });
    }
    فراغ گهمو(AstVarRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            إذا (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وإلا {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_اشارو().asString();
            }
        });
    }
    فراغ گهمو(AstVarXRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstFScanF* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ گهمو(AstSScanF* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ گهمو(Ast_Addr_جو_CFunc* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ گهمو(AstTriggered* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            لكل (ثابت uint32_t اشاريو : nodep->indices()) m_hash += اشاريو;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    فراغ گهمو(Ast_ڳنڍ_Stmt* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(Ast_ڳنڍ_CCall* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ گهمو(Ast_Case_شي* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(Ast_ڳنڍ_FTask_Ref* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    فراغ گهمو(AstCMethodHard* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->طريقو();
        });
    }
    فراغ گهمو(AstCAwait* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ گهمو(AstCLocalScope* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstCoverInc* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ گهمو(AstDisplay* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ گهمو(AstMonitorOff* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    فراغ گهمو(AstJumpGo* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, خطأ, []() {});
    }
    فراغ گهمو(AstTraceInc* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ گهمو(Ast_ڳنڍ_Cover_Or_Assert* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    فراغ گهمو(Ast_ڳنڍ_Range* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(Ast_ڳنڍ_Pre_Sel* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstClassExtends* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(Ast_Foreach_سرو* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstDefParam* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstArg* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstParseRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    فراغ گهمو(Ast_Sen_شي* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->ڪنارو_Type();
        });
    }
    فراغ گهمو(Ast_Sen_وڻ* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstSFormatF* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ گهمو(AstElabDisplay* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ گهمو(Ast_ڳنڍ_Cons_شي* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ خطأ,
                                     [هذا, nodep, dtypep]() {
                                         ثابت uint32_t سائيز = dtypep->elementsConst();
                                         لكل (uint32_t n = 0; n < سائيز; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    فراغ گهمو(AstPragma* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    فراغ گهمو(Ast_Attr_جو* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    فراغ گهمو(Ast_ڳنڍ_File* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstCFunc* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                إذا (m_cache_اندريون_User4) nodep->user4(V3Hash{1}.قدر());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    فراغ گهمو(AstVar* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ گهمو(AstScope* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    فراغ گهمو(AstVarScope* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    فراغ گهمو(Ast_Enum_شي* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstTypedef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstTypedefFwd* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(Ast_سرگرم* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ گهمو(AstCell* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    فراغ گهمو(AstCellInline* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [هذا, nodep]() { m_hash += nodep->name(); });
    }
    فراغ گهمو(Ast_ڳنڍ_FTask* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                إذا (m_cache_اندريون_User4) nodep->user4(V3Hash{1}.قدر());
            });
        });
    }
    فراغ گهمو(AstModport* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstModportVarRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    فراغ گهمو(AstModportFTaskRef* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    فراغ گهمو(Ast_ڳنڍ_Procedure* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstBegin* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ گهمو(AstFork* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->فوري_شروعات();
        });
    }
    فراغ گهمو(AstPin* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_انگ();
        });
    }
    فراغ گهمو(Ast_متن* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ گهمو(Ast_متن_بلاڪ* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->اڳياڙي();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    فراغ گهمو(AstCStmt* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstCStmtUser* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstCExpr* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstCExprUser* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ گهمو(AstWith* nodep) override {
        m_hash += hash_ڳنڍ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عام:
    // CONSTRUCTORS
    واضح HasherVisitor(Ast_ڳنڍ* nodep)
        : m_cache_اندريون_User4{سچ} {
        iterateConst(nodep);
    }
    صنف Uncached {};
    HasherVisitor(ثابت Ast_ڳنڍ* nodep, Uncached)
        : m_cache_اندريون_User4{خطأ} {
        iterateConst(const_cast<Ast_ڳنڍ*>(nodep));
    }
    V3Hash finalHash() ثابت { أرجع m_hash; }
    ~HasherVisitor() override = افتراضي;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::عامل()(Ast_ڳنڍ* nodep) ثابت {
    إذا (!nodep->user4()) HasherVisitor{nodep};
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ڳنڍ* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_ڳنڍ* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    أرجع visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

سائيز_t V3HasherUncachedHash(ثابت Ast_ڳنڍ& ڳنڍ) {
    أرجع static_cast<سائيز_t>(V3Hasher::uncachedHash(&ڳنڍ).قدر());
}
