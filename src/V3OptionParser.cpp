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

#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
#define VL_MT_DISABLED_CODE_UNIT 1

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Options.h"
#endif
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3OptionParser.h"
#अन्तर्भाव "V3String.h"

//######################################################################
// V3OptionParser::Impl
संरचना V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    गणना वर्ग en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        मूल्य_3  // "-opt val"
    };
    // Base class of actual action classes
    ढाँचा <en N_Mode, तार्किक N_Allow_Partial_Match = असत्य>
    वर्ग ActionBase VL_NOT_FINAL : सार्वजनिक ActionIfs {
        तार्किक m_notForRerun = असत्य;  // This option is not needed for rerun with --dump-inputs
        तार्किक m_undocumented = असत्य;  // This option is not documented
    सार्वजनिक:
        तार्किक is_मूल्य_Needed() स्थिरांक override final { परत N_Mode == en::मूल्य_3; }
        तार्किक isFOnOffAllowed() स्थिरांक override final { परत N_Mode == en::FONOFF; }
        तार्किक isNotForRerun() स्थिरांक override final { परत m_notForRerun; }
        तार्किक isOnOffAllowed() स्थिरांक override final { परत N_Mode == en::ONOFF; }
        तार्किक isPartialMatchAllowed() स्थिरांक override final { परत N_Allow_Partial_Match; }
        तार्किक isUndocumented() स्थिरांक override { परत m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = सत्य;
            परत *हे;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = सत्य;
            परत *हे;
        }
    };

    // Actual action classes
    ढाँचा <typename T>
    वर्ग ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    ढाँचा <typename N_Bool>
    वर्ग ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    ढाँचा <typename N_Bool>
    वर्ग ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    वर्ग ActionCbCall;  // Callback without argument for "-opt"
    वर्ग ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    वर्ग ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    ढाँचा <typename T>
    वर्ग ActionCbVal;  // Callback for "-opt val"
    वर्ग ActionCbPartialMatch;  // Callback "-O3" for "-O"
    वर्ग ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<स्थिरांक string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    तार्किक m_isFinalized{असत्य};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    ढाँचा <> \
    वर्ग V3OptionParser::Impl::className<type> final : सार्वजनिक ActionBase<enType> { \
        type* स्थिरांक m_valp; /* Pointer to an option variable*/ \
\
    सार्वजनिक: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        शून्य exec(स्थिरांक अक्षर* optp, स्थिरांक अक्षर* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, तार्किक, *m_valp = सत्य, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(सत्य), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, पूर्णांक, *m_valp = std::म_से_प(argp), en::मूल्य_3);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::मूल्य_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, तार्किक, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, तार्किक, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    वर्ग V3OptionParser::Impl::className final : सार्वजनिक ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    सार्वजनिक: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        शून्य exec(स्थिरांक अक्षर* optp, स्थिरांक अक्षर* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, शून्य(शून्य), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, शून्य(तार्किक), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, शून्य(तार्किक), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, शून्य(स्थिरांक अक्षर*), m_cb(optp), en::NONE,
                                 सत्य);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, शून्य(स्थिरांक अक्षर*, स्थिरांक अक्षर*),
                                 m_cb(optp, argp), en::मूल्य_3, सत्य);

ढाँचा <>
वर्ग V3OptionParser::Impl::ActionCbVal<पूर्णांक> final : सार्वजनिक ActionBase<en::मूल्य_3> {
    std::function<शून्य(पूर्णांक)> m_cb;  // Callback function

सार्वजनिक:
    using CbType = std::function<शून्य(पूर्णांक)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(स्थिरांक अक्षर* optp, स्थिरांक अक्षर* argp) override { m_cb(std::म_से_प(argp)); }
};

ढाँचा <>
वर्ग V3OptionParser::Impl::ActionCbVal<स्थिरांक अक्षर*> final : सार्वजनिक ActionBase<en::मूल्य_3> {
    std::function<शून्य(स्थिरांक अक्षर*)> m_cb;

सार्वजनिक:
    using CbType = std::function<शून्य(स्थिरांक अक्षर*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(स्थिरांक अक्षर* optp, स्थिरांक अक्षर* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(स्थिरांक अक्षर* optp) {
    स्थिरांक स्वतः it = m_pimpl->m_options.find(optp);
    जर (it != m_pimpl->m_options.end()) परत it->second.get();  // Exact match
    साठी (स्वतः&& act : m_pimpl->m_options) {
        जर (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            जर (स्थिरांक अक्षर* स्थिरांक nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::म_माप("-fno-")) : nullptr) {
                जर (act.first.substr(std::म_माप("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    परत act.second.get();
                }
            }
        }
        जर (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            जर (स्थिरांक अक्षर* स्थिरांक nop
                = VString::startsWith(optp, "-no") ? (optp + std::म_माप("-no")) : nullptr) {
                जर (act.first == nop || act.first == ("-"s + nop)) परत act.second.get();
            }
        } अन्यथा जर (act.second->isPartialMatchAllowed()) {
            जर (VString::startsWith(optp, act.first)) परत act.second.get();
        }
    }
    परत nullptr;
}

ढाँचा <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(स्थिरांक std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{नवीन T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    स्थिरांक स्वतः insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    परत *insertedResult.first->second;
}

तार्किक V3OptionParser::hasPrefixFNo(स्थिरांक अक्षर* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    जर (strp[1] == '-') ++strp;
    परत VString::startsWith(strp, "-fno");
}

तार्किक V3OptionParser::hasPrefixNo(स्थिरांक अक्षर* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    जर (strp[1] == '-') ++strp;
    परत VString::startsWith(strp, "-no");
}

std::pair<पूर्णांक, तार्किक> V3OptionParser::parse(पूर्णांक idx, पूर्णांक argc, अक्षर* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    स्थिरांक अक्षर* optp = argv[idx];
    जर (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* स्थिरांक actp = find(optp);
    जर (!actp) परत {0, असत्य};
    जर (!actp->is_मूल्य_Needed()) {
        actp->exec(optp, nullptr);
        परत {1, !actp->isNotForRerun()};
    }
    जर (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        परत {2, !actp->isNotForRerun()};
    }
    परत {0, असत्य};
}

string V3OptionParser::getSuggestion(स्थिरांक अक्षर* str) स्थिरांक {
    परत m_pimpl->m_spellCheck.best_Candidate_संदेश(str);
}

शून्य V3OptionParser::addSuggestionCandidate(स्थिरांक string& s) {
    m_pimpl->m_spellCheck.ढकला_Candidate(s);
}

शून्य V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    साठी (स्वतः&& opt : m_pimpl->m_options) {
        जर (opt.second->isUndocumented()) सुरूठेवा;
        m_pimpl->m_spellCheck.ढकला_Candidate(opt.first);
        जर (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.ढकला_Candidate(
                "-fno-" + opt.first.substr(std::म_माप("-f"), std::string::npos));
        }
        जर (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.ढकला_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = सत्य;
}

शून्य V3OptionParser::dumpOptions() {
    साठी (स्थिरांक स्वतः& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{नवीन Impl{}} {}

V3OptionParser::~V3OptionParser() = पूर्वनिर्धारित;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::चालक()( \
        स्थिरांक अक्षर* optp, actKind, argType arg) स्थिरांक { \
        परत m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, तार्किक*, ActionSet<तार्किक>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, पूर्णांक*, ActionSet<पूर्णांक>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, तार्किक*, ActionFOnOff<तार्किक>)
V3OPTION_PARSER_DEF_OP(OnOff, तार्किक*, ActionOnOff<तार्किक>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<पूर्णांक>::CbType, ActionCbVal<पूर्णांक>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<स्थिरांक अक्षर*>::CbType, ActionCbVal<स्थिरांक अक्षर*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::चालक()(स्थिरांक अक्षर* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) स्थिरांक {
    स्थिरांक माप_प्रकार prefixLen = std::म_माप(optp);
    स्थिरांक स्वतः wrap = [prefixLen, cb](स्थिरांक अक्षर* optp) { cb(optp + prefixLen); };
    परत m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::चालक()(स्थिरांक अक्षर* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) स्थिरांक {
    स्थिरांक माप_प्रकार prefixLen = std::म_माप(optp);
    स्थिरांक स्वतः wrap
        = [prefixLen, cb](स्थिरांक अक्षर* optp, स्थिरांक अक्षर* argp) { cb(optp + prefixLen, argp); };
    परत m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
