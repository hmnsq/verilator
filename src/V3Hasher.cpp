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

#شامل "V3PchAstMT.h"

#شامل "V3Hasher.h"

#شامل <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

جماعت HasherVisitor final : عوامی VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ثابت منطقی m_cache_اندر_User4;  // Use user4 to cache each V3Hash?
    std::سیٹ<Ast_گرہ*> m_دیکھا;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    خالی guardRecursion(Ast_گرہ* ثابت nodep, std::function<خالی()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        اگر (!m_cache_اندر_User4) {
            اگر (m_دیکھا.ڈھونڈو(nodep) != m_دیکھا.انتہا()) {
                m_hash += V3Hash{nodep->name()};
                واپس;
            }
            m_دیکھا.داخل_کرو(nodep);
        }
        f();
    }

    V3Hash hash_گرہ_And_Iterate(Ast_گرہ* nodep, منطقی hashDType, منطقی hashChildren,
                              std::function<خالی()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        اگر (m_cache_اندر_User4 && nodep->user4()) {
            واپس V3Hash{static_cast<uint32_t>(nodep->user4())};
        } ورنہ {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            اگر (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            اگر (hashChildren) iterateChildrenConst(nodep);  // Children
            اگر (m_cache_اندر_User4) nodep->user4(m_hash.قدر());
            واپس m_hash;
        }
    }

    // VISITORS

    constexpr ساکن منطقی HASH_DTYPE = سچ;
    constexpr ساکن منطقی HASH_CHILDREN = سچ;

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

    خالی دورہ(Ast_گرہ* nodep) override {
#اگر VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#اگر_ختم
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    خالی دورہ(Ast_گرہ_Array_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->بائیں();
            m_hash += nodep->دائیں();
        });
    }
    خالی دورہ(Ast_گرہ_UOr_Struct_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, جھوٹ, [یہ, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    خالی دورہ(AstParamTypeDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    خالی دورہ(AstMemberDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstDefImplicitDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }
    خالی دورہ(AstAssocArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    خالی دورہ(AstBracketArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstDynArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstWildcardArrayDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstBasicDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().بائیں();
            m_hash += nodep->nrange().دائیں();
        });
    }
    خالی دورہ(AstCDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstConstDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstClassRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    خالی دورہ(AstIfaceRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    خالی دورہ(Ast_قطار_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    خالی دورہ(AstRefDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    خالی دورہ(Ast_دھارا_DType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstVoidDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstEnumDType* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, جھوٹ, [یہ, nodep]() {  //
            m_hash += nodep->unique_عدد();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    خالی دورہ(Ast_گرہ_اظہاریہ* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstSel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->چوڑائی_Const();
        });
    }
    خالی دورہ(AstConst* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->عدد().مقصد_Hash();
        });
    }
    خالی دورہ(AstNullCheck* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstCCast* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->سائز();
        });
    }
    خالی دورہ(AstVarRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            اگر (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } ورنہ {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_اشارہ_گر().asString();
            }
        });
    }
    خالی دورہ(AstVarXRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    خالی دورہ(AstMemberSel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstFScanF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    خالی دورہ(AstSScanF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    خالی دورہ(Ast_Addr_کا_CFunc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    خالی دورہ(AstTriggered* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            برائے (ثابت uint32_t اشاریہ : nodep->indices()) m_hash += اشاریہ;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    خالی دورہ(Ast_گرہ_Stmt* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(Ast_گرہ_CCall* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    خالی دورہ(Ast_Case_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(Ast_گرہ_FTask_Ref* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    خالی دورہ(AstCMethodHard* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->طریقہ();
        });
    }
    خالی دورہ(AstCAwait* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    خالی دورہ(AstCLocalScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstCoverInc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    خالی دورہ(AstDisplay* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    خالی دورہ(AstMonitorOff* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    خالی دورہ(AstJumpGo* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, جھوٹ, []() {});
    }
    خالی دورہ(AstTraceInc* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    خالی دورہ(Ast_گرہ_Cover_Or_Assert* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    خالی دورہ(Ast_گرہ_Range* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(Ast_گرہ_Module* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, جھوٹ, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(Ast_گرہ_Pre_Sel* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstClassExtends* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(Ast_Foreach_سرخی* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstDefParam* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstArg* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstParseRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstClassOrPackageRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    خالی دورہ(Ast_Sen_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->کنارہ_Type();
        });
    }
    خالی دورہ(Ast_Sen_درخت* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstSFormatF* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    خالی دورہ(AstElabDisplay* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    خالی دورہ(Ast_گرہ_Cons_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ جھوٹ,
                                     [یہ, nodep, dtypep]() {
                                         ثابت uint32_t سائز = dtypep->elementsConst();
                                         برائے (uint32_t n = 0; n < سائز; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    خالی دورہ(AstPragma* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    خالی دورہ(Ast_Attr_کا* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    خالی دورہ(Ast_گرہ_File* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstCFunc* nodep) override {
        guardRecursion(nodep, [یہ, nodep]() {  //
            m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                اگر (m_cache_اندر_User4) nodep->user4(V3Hash{1}.قدر());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    خالی دورہ(AstVar* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    خالی دورہ(AstScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, جھوٹ, [یہ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    خالی دورہ(AstVarScope* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    خالی دورہ(Ast_Enum_شے* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstTypedef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstTypedefFwd* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(Ast_فعال* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    خالی دورہ(AstCell* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    خالی دورہ(AstCellInline* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [یہ, nodep]() { m_hash += nodep->name(); });
    }
    خالی دورہ(Ast_گرہ_FTask* nodep) override {
        guardRecursion(nodep, [یہ, nodep]() {  //
            m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                اگر (m_cache_اندر_User4) nodep->user4(V3Hash{1}.قدر());
            });
        });
    }
    خالی دورہ(AstModport* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstModportVarRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    خالی دورہ(AstModportFTaskRef* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    خالی دورہ(Ast_گرہ_Procedure* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstBegin* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    خالی دورہ(AstFork* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->فوری_آغاز();
        });
    }
    خالی دورہ(AstPin* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [یہ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_عدد();
        });
    }
    خالی دورہ(Ast_متن* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    خالی دورہ(Ast_متن_بلاک* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, [یہ, nodep]() {  //
            m_hash += nodep->سابقہ();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    خالی دورہ(AstCStmt* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstCStmtUser* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, جھوٹ, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstCExpr* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstCExprUser* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    خالی دورہ(AstWith* nodep) override {
        m_hash += hash_گرہ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عوامی:
    // CONSTRUCTORS
    صریح HasherVisitor(Ast_گرہ* nodep)
        : m_cache_اندر_User4{سچ} {
        iterateConst(nodep);
    }
    جماعت Uncached {};
    HasherVisitor(ثابت Ast_گرہ* nodep, Uncached)
        : m_cache_اندر_User4{جھوٹ} {
        iterateConst(const_cast<Ast_گرہ*>(nodep));
    }
    V3Hash finalHash() ثابت { واپس m_hash; }
    ~HasherVisitor() override = طےشدہ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::عامل()(Ast_گرہ* nodep) ثابت {
    اگر (!nodep->user4()) HasherVisitor{nodep};
    واپس V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_گرہ* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    واپس V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_گرہ* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    واپس visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

سائز_t V3HasherUncachedHash(ثابت Ast_گرہ& گرہ) {
    واپس static_cast<سائز_t>(V3Hasher::uncachedHash(&گرہ).قدر());
}
