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
    ثابت منطقي m_cache_درون_User4;  // Use user4 to cache each V3Hash?
    std::مجموعه<Ast_گره*> m_دیدهشده;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    فراغ guardRecursion(Ast_گره* ثابت nodep, std::function<فراغ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        إذا (!m_cache_درون_User4) {
            إذا (m_دیدهشده.پیدا_کن(nodep) != m_دیدهشده.انتها()) {
                m_hash += V3Hash{nodep->name()};
                أرجع;
            }
            m_دیدهشده.درج(nodep);
        }
        f();
    }

    V3Hash hash_گره_And_Iterate(Ast_گره* nodep, منطقي hashDType, منطقي hashChildren,
                              std::function<فراغ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        إذا (m_cache_درون_User4 && nodep->user4()) {
            أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
        } وإلا {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            إذا (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            إذا (hashChildren) iterateChildrenConst(nodep);  // Children
            إذا (m_cache_درون_User4) nodep->user4(m_hash.مقدار());
            أرجع m_hash;
        }
    }

    // VISITORS

    constexpr ساكن منطقي HASH_DTYPE = درست;
    constexpr ساكن منطقي HASH_CHILDREN = درست;

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

    فراغ بازدید(Ast_گره* nodep) override {
#إذا VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#پایان_اگر
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    فراغ بازدید(Ast_گره_Array_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->چپ();
            m_hash += nodep->راست();
        });
    }
    فراغ بازدید(Ast_گره_UOr_Struct_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }
    فراغ بازدید(AstParamTypeDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ بازدید(AstMemberDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstDefImplicitDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }
    فراغ بازدید(AstAssocArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    فراغ بازدید(AstBracketArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstDynArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstWildcardArrayDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstBasicDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().چپ();
            m_hash += nodep->nrange().راست();
        });
    }
    فراغ بازدید(AstCDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstConstDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstClassRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    فراغ بازدید(AstIfaceRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    فراغ بازدید(Ast_صف_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    فراغ بازدید(AstRefDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    فراغ بازدید(Ast_جریان_DType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstVoidDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstEnumDType* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->unique_رقم();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstSel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->عرض_Const();
        });
    }
    فراغ بازدید(AstConst* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->رقم().مقصد_Hash();
        });
    }
    فراغ بازدید(AstNullCheck* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstCCast* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->اندازه();
        });
    }
    فراغ بازدید(AstVarRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            إذا (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } وإلا {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_اشاره_گر().asString();
            }
        });
    }
    فراغ بازدید(AstVarXRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstFScanF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ بازدید(AstSScanF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ بازدید(Ast_Addr_از_CFunc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ بازدید(AstTriggered* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            لكل (ثابت uint32_t شاخص : nodep->indices()) m_hash += شاخص;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    فراغ بازدید(Ast_گره_Stmt* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(Ast_گره_CCall* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    فراغ بازدید(Ast_Case_قلم* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    فراغ بازدید(AstCMethodHard* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->روش();
        });
    }
    فراغ بازدید(AstCAwait* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ بازدید(AstCLocalScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstCoverInc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ بازدید(AstDisplay* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ بازدید(AstMonitorOff* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    فراغ بازدید(AstJumpGo* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, خطأ, []() {});
    }
    فراغ بازدید(AstTraceInc* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    فراغ بازدید(Ast_گره_Cover_Or_Assert* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    فراغ بازدید(Ast_گره_Range* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(Ast_گره_Pre_Sel* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstClassExtends* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(Ast_Foreach_سرآیند* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstDefParam* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstArg* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstParseRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstClassOrPackageRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->یال_Type();
        });
    }
    فراغ بازدید(Ast_Sen_درخت* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstSFormatF* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ بازدید(AstElabDisplay* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    فراغ بازدید(Ast_گره_Cons_قلم* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ثابت AstUnpackArrayDType* ثابت dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ خطأ,
                                     [هذا, nodep, dtypep]() {
                                         ثابت uint32_t اندازه = dtypep->elementsConst();
                                         لكل (uint32_t n = 0; n < اندازه; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    فراغ بازدید(AstPragma* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    فراغ بازدید(Ast_Attr_از* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    فراغ بازدید(Ast_گره_File* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstCFunc* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                إذا (m_cache_درون_User4) nodep->user4(V3Hash{1}.مقدار());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    فراغ بازدید(AstVar* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    فراغ بازدید(AstScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, خطأ, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    فراغ بازدید(AstVarScope* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    فراغ بازدید(Ast_Enum_قلم* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstTypedef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstTypedefFwd* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(Ast_فعال* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    فراغ بازدید(AstCell* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    فراغ بازدید(AstCellInline* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [هذا, nodep]() { m_hash += nodep->name(); });
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        guardRecursion(nodep, [هذا, nodep]() {  //
            m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                إذا (m_cache_درون_User4) nodep->user4(V3Hash{1}.مقدار());
            });
        });
    }
    فراغ بازدید(AstModport* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstModportVarRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    فراغ بازدید(AstModportFTaskRef* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    فراغ بازدید(Ast_گره_Procedure* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstBegin* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    فراغ بازدید(AstFork* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->فوری_شروع();
        });
    }
    فراغ بازدید(AstPin* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [هذا, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_رقم();
        });
    }
    فراغ بازدید(Ast_متن* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->متن();
        });
    }
    فراغ بازدید(Ast_متن_بلوک* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, [هذا, nodep]() {  //
            m_hash += nodep->پیشوند();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    فراغ بازدید(AstCStmt* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstCStmtUser* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, خطأ, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstCExpr* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstCExprUser* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    فراغ بازدید(AstWith* nodep) override {
        m_hash += hash_گره_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

عام:
    // CONSTRUCTORS
    صریح HasherVisitor(Ast_گره* nodep)
        : m_cache_درون_User4{درست} {
        iterateConst(nodep);
    }
    صنف Uncached {};
    HasherVisitor(ثابت Ast_گره* nodep, Uncached)
        : m_cache_درون_User4{خطأ} {
        iterateConst(const_cast<Ast_گره*>(nodep));
    }
    V3Hash finalHash() ثابت { أرجع m_hash; }
    ~HasherVisitor() override = افتراضي;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::عملگر()(Ast_گره* nodep) ثابت {
    إذا (!nodep->user4()) HasherVisitor{nodep};
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_گره* nodep) ثابت {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    أرجع V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ثابت Ast_گره* nodep) {
    ثابت HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    أرجع visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

اندازه_t V3HasherUncachedHash(ثابت Ast_گره& گره) {
    أرجع static_cast<اندازه_t>(V3Hasher::uncachedHash(&گره).مقدار());
}
