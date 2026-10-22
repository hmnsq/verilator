// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#اگر_غیر_تعریف_شدہ VERILATOR_V3HIERBLOCK_H_
#تعریف_کرو VERILATOR_V3HIERBLOCK_H_

#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Graph.h"

#شامل <map>
#شامل <set>
#شامل <string>
#شامل <unordered_map>
#شامل <unordered_set>
#شامل <utility>
#شامل <vector>

class AstNetlist;
class Ast_گرہ_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3_Hier_گراف final : عوامی V3_گراف {
عوامی:
    V3_Hier_گراف() = طےشدہ;
    ~V3_Hier_گراف() = طےشدہ;
    VL_UNCOPYABLE(V3_Hier_گراف);
    VL_UNMOVABLE(V3_Hier_گراف);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ساکن VStringList ماخذ_Files(ثابت string& چوٹی_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    خالی write_Command_دلائل_Files(bool forMkJson) ثابت VL_MT_DISABLED;
    خالی writeParametersFiles() ثابت VL_MT_DISABLED;
    ساکن string چوٹی_Command_دلائل_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3_Hier_بلاک final : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(V3_Hier_بلاک, V3_گراف_راس)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::سمتیہ<StrGParam>;

    // MEMBERS
    ثابت AstModule* ثابت m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ثابت std::سمتیہ<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ثابت std::سمتیہ<AstParamTypeDType*> m_typeParams;

    // METHODS
    ساکن StrGParams stringifyParams(ثابت std::سمتیہ<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

عوامی:
    // CONSTRUCTORs
    V3_Hier_بلاک(V3_Hier_گراف* graphp, ثابت AstModule* modp, ثابت std::سمتیہ<AstVar*>& params,
                ثابت std::سمتیہ<AstParamTypeDType*>& typeParams)
        : V3_گراف_راس{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3_Hier_بلاک() VL_MT_DISABLED = طےشدہ;
    VL_UNCOPYABLE(V3_Hier_بلاک);
    VL_UNMOVABLE(V3_Hier_بلاک);

    ثابت AstModule* modp() ثابت { واپس m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ساکن bool stringParamPassable(ثابت string& قدر) VL_خالص;

    // For emitting Makefile and build definition JSON
    VStringList command_دلائل(bool forMkJson) ثابت VL_MT_DISABLED;
    VStringList hier_بلاک_دلائل() ثابت VL_MT_DISABLED;
    string hier_سابقہ() ثابت VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ثابت حرف* سابقہ,
                            ثابت حرف* suffix) ثابت VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ثابت VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ثابت VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ثابت VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    خالی write_Command_دلائل_File(bool forMkJson) ثابت VL_MT_DISABLED;
    خالی writeParametersFile() ثابت VL_MT_DISABLED;
    string command_دلائل_Filename(bool forMkJson) ثابت VL_MT_DISABLED;
    string typeParametersFilename() ثابت VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ثابت override { واپس m_modp->prettyNameQ(); }
    std::string dotShape() ثابت override { واپس "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3_درجہ_وار final {
عوامی:
    ساکن خالی create_گراف(AstNetlist* nodep) VL_MT_DISABLED;
};

#اگر_ختم  // guard
