// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line option parser
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

#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "V3Global.h"
#समावेश "V3Options.h"
#पूर्ण_अगर
#समावेश "V3Error.h"
#समावेश "V3OptionParser.h"
#समावेश "V3String.h"

//######################################################################
// V3OptionParser::Impl
काष्ठा V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    क्रमागत श्रेणी en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        मूल्य_3  // "-opt val"
    };
    // Base class of actual action classes
    ढाँचा <en N_Mode, bool N_Allow_Partial_Match = false>
    श्रेणी ActionBase VL_NOT_FINAL : खुला ActionIfs {
        bool m_notForRerun = false;  // This option is not needed for rerun with --dump-inputs
        bool m_undocumented = false;  // This option is not documented
    खुला:
        bool is_मूल्य_Needed() स्थिर override final { वापस N_Mode == en::मूल्य_3; }
        bool isFOnOffAllowed() स्थिर override final { वापस N_Mode == en::FONOFF; }
        bool isNotForRerun() स्थिर override final { वापस m_notForRerun; }
        bool isOnOffAllowed() स्थिर override final { वापस N_Mode == en::ONOFF; }
        bool isPartialMatchAllowed() स्थिर override final { वापस N_Allow_Partial_Match; }
        bool isUndocumented() स्थिर override { वापस m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = true;
            वापस *यह;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = true;
            वापस *यह;
        }
    };

    // Actual action classes
    ढाँचा <typename T>
    श्रेणी ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    ढाँचा <typename N_Bool>
    श्रेणी ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    ढाँचा <typename N_Bool>
    श्रेणी ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    श्रेणी ActionCbCall;  // Callback without argument for "-opt"
    श्रेणी ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    श्रेणी ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    ढाँचा <typename T>
    श्रेणी ActionCbVal;  // Callback for "-opt val"
    श्रेणी ActionCbPartialMatch;  // Callback "-O3" for "-O"
    श्रेणी ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<स्थिर string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    bool m_isFinalized{false};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#घोषणा V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    ढाँचा <> \
    श्रेणी V3OptionParser::Impl::className<type> final : खुला ActionBase<enType> { \
        type* स्थिर m_valp; /* Pointer to an option variable*/ \
\
    खुला: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        व्योम exec(स्थिर अक्षर* optp, स्थिर अक्षर* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, bool, *m_valp = true, en::NONE);
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(true), en::NONE);
#पूर्ण_अगर
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, पूर्णांक, *m_valp = std::म_से_प(argp), en::मूल्य_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::मूल्य_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, bool, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, bool, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#पूर्ण_अगर
#अघोषित V3OPTION_PARSER_DEF_ACT_CLASS

#घोषणा V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    श्रेणी V3OptionParser::Impl::className final : खुला ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    खुला: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        व्योम exec(स्थिर अक्षर* optp, स्थिर अक्षर* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, व्योम(व्योम), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, व्योम(bool), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, व्योम(bool), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, व्योम(स्थिर अक्षर*), m_cb(optp), en::NONE,
                                 true);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, व्योम(स्थिर अक्षर*, स्थिर अक्षर*),
                                 m_cb(optp, argp), en::मूल्य_3, true);

ढाँचा <>
श्रेणी V3OptionParser::Impl::ActionCbVal<पूर्णांक> final : खुला ActionBase<en::मूल्य_3> {
    std::function<व्योम(पूर्णांक)> m_cb;  // Callback function

खुला:
    using CbType = std::function<व्योम(पूर्णांक)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    व्योम exec(स्थिर अक्षर* optp, स्थिर अक्षर* argp) override { m_cb(std::म_से_प(argp)); }
};

ढाँचा <>
श्रेणी V3OptionParser::Impl::ActionCbVal<स्थिर अक्षर*> final : खुला ActionBase<en::मूल्य_3> {
    std::function<व्योम(स्थिर अक्षर*)> m_cb;

खुला:
    using CbType = std::function<व्योम(स्थिर अक्षर*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    व्योम exec(स्थिर अक्षर* optp, स्थिर अक्षर* argp) override { m_cb(argp); }
};

#अघोषित V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(स्थिर अक्षर* optp) {
    स्थिर स्वतः it = m_pimpl->m_options.find(optp);
    अगर (it != m_pimpl->m_options.end()) वापस it->second.लो();  // Exact match
    क्रम (स्वतः&& act : m_pimpl->m_options) {
        अगर (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            अगर (स्थिर अक्षर* स्थिर nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::म_माप("-fno-")) : nullptr) {
                अगर (act.first.substr(std::म_माप("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    वापस act.second.लो();
                }
            }
        }
        अगर (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            अगर (स्थिर अक्षर* स्थिर nop
                = VString::startsWith(optp, "-no") ? (optp + std::म_माप("-no")) : nullptr) {
                अगर (act.first == nop || act.first == ("-"s + nop)) वापस act.second.लो();
            }
        } अन्यथा अगर (act.second->isPartialMatchAllowed()) {
            अगर (VString::startsWith(optp, act.first)) वापस act.second.लो();
        }
    }
    वापस nullptr;
}

ढाँचा <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(स्थिर std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{नया T_Act{std::move(arg)}};
    UASSERT(opt.आकार() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    स्थिर स्वतः inserted_परिणाम = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_परिणाम.second, opt << " is already registered");
    वापस *inserted_परिणाम.first->second;
}

bool V3OptionParser::hasPrefixFNo(स्थिर अक्षर* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    अगर (strp[1] == '-') ++strp;
    वापस VString::startsWith(strp, "-fno");
}

bool V3OptionParser::hasPrefixNo(स्थिर अक्षर* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    अगर (strp[1] == '-') ++strp;
    वापस VString::startsWith(strp, "-no");
}

std::pair<पूर्णांक, bool> V3OptionParser::parse(पूर्णांक idx, पूर्णांक argc, अक्षर* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    स्थिर अक्षर* optp = argv[idx];
    अगर (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* स्थिर actp = find(optp);
    अगर (!actp) वापस {0, false};
    अगर (!actp->is_मूल्य_Needed()) {
        actp->exec(optp, nullptr);
        वापस {1, !actp->isNotForRerun()};
    }
    अगर (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        वापस {2, !actp->isNotForRerun()};
    }
    वापस {0, false};
}

string V3OptionParser::getSuggestion(स्थिर अक्षर* str) स्थिर {
    वापस m_pimpl->m_spellCheck.best_Candidate_संदेश(str);
}

व्योम V3OptionParser::addSuggestionCandidate(स्थिर string& s) {
    m_pimpl->m_spellCheck.धकेलो_Candidate(s);
}

व्योम V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    क्रम (स्वतः&& opt : m_pimpl->m_options) {
        अगर (opt.second->isUndocumented()) जारी;
        m_pimpl->m_spellCheck.धकेलो_Candidate(opt.first);
        अगर (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.धकेलो_Candidate(
                "-fno-" + opt.first.substr(std::म_माप("-f"), std::string::npos));
        }
        अगर (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.धकेलो_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = true;
}

व्योम V3OptionParser::dumpOptions() {
    क्रम (स्थिर स्वतः& opt : m_pimpl->m_spellCheck.candidates()) {
        std::अ_बाहर << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{नया Impl{}} {}

V3OptionParser::~V3OptionParser() = शेष;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#घोषणा V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::चालक()( \
        स्थिर अक्षर* optp, actKind, argType arg) स्थिर { \
        वापस m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, bool*, ActionSet<bool>)
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#पूर्ण_अगर
V3OPTION_PARSER_DEF_OP(Set, पूर्णांक*, ActionSet<पूर्णांक>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, bool*, ActionFOnOff<bool>)
V3OPTION_PARSER_DEF_OP(OnOff, bool*, ActionOnOff<bool>)
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#पूर्ण_अगर
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<पूर्णांक>::CbType, ActionCbVal<पूर्णांक>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<स्थिर अक्षर*>::CbType, ActionCbVal<स्थिर अक्षर*>)
#अघोषित V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::चालक()(स्थिर अक्षर* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) स्थिर {
    स्थिर माप_प्रकार prefixLen = std::म_माप(optp);
    स्थिर स्वतः wrap = [prefixLen, cb](स्थिर अक्षर* optp) { cb(optp + prefixLen); };
    वापस m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::चालक()(स्थिर अक्षर* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) स्थिर {
    स्थिर माप_प्रकार prefixLen = std::म_माप(optp);
    स्थिर स्वतः wrap
        = [prefixLen, cb](स्थिर अक्षर* optp, स्थिर अक्षर* argp) { cb(optp + prefixLen, argp); };
    वापस m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
