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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3HIERBLOCK_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3HIERBLOCK_H_

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class AstNetlist;
class Ast_ಗಂಟು_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3_Hier_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
ಸಾರ್ವಜನಿಕ:
    V3_Hier_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3_Hier_ನಕ್ಷೆ() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(V3_Hier_ನಕ್ಷೆ);
    VL_UNMOVABLE(V3_Hier_ನಕ್ಷೆ);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    ಸ್ಥಿರ VStringList ಮೂಲ_Files(ಸ್ಥಿರಾಂಕ string& ತುದಿ_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    ಶೂನ್ಯ write_Command_ವಾದಗಳು_Files(bool forMkJson) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ writeParametersFiles() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಸ್ಥಿರ string ತುದಿ_Command_ವಾದಗಳು_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3_Hier_ಖಂಡ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(V3_Hier_ಖಂಡ, V3_ನಕ್ಷೆ_ಶೃಂಗ)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::ಸದಿಶ<StrGParam>;

    // MEMBERS
    ಸ್ಥಿರಾಂಕ AstModule* ಸ್ಥಿರಾಂಕ m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstParamTypeDType*> m_typeParams;

    // METHODS
    ಸ್ಥಿರ StrGParams stringifyParams(ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORs
    V3_Hier_ಖಂಡ(V3_Hier_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ AstModule* modp, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstVar*>& params,
                ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<AstParamTypeDType*>& typeParams)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3_Hier_ಖಂಡ() VL_MT_DISABLED = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(V3_Hier_ಖಂಡ);
    VL_UNMOVABLE(V3_Hier_ಖಂಡ);

    ಸ್ಥಿರಾಂಕ AstModule* modp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    ಸ್ಥಿರ bool stringParamPassable(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) VL_ಶುದ್ಧ;

    // For emitting Makefile and build definition JSON
    VStringList command_ವಾದಗಳು(bool forMkJson) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    VStringList hier_ಖಂಡ_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hier_ಪೂರ್ವಪ್ರತ್ಯಯ() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಪೂರ್ವಪ್ರತ್ಯಯ,
                            ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* suffix) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    ಶೂನ್ಯ write_Command_ವಾದಗಳು_File(bool forMkJson) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ writeParametersFile() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string command_ವಾದಗಳು_Filename(bool forMkJson) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    string typeParametersFilename() ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ m_modp->prettyNameQ(); }
    std::string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3_ಶ್ರೇಣೀಕೃತ final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ create_ನಕ್ಷೆ(AstNetlist* nodep) VL_MT_DISABLED;
};

#ಆದರೆ_ಅಂತ್ಯ  // guard
