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

#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
#परिभाषय VL_MT_DISABLED_संहिता_UNIT 1

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Options.h"
#यदि_अन्तः
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
        मानम्_3  // "-opt val"
    };
    // Base class of actual action classes
    प्रतिरूपम् <en N_बहुलकम्, तार्किक N_Allow_Partial_मेलनम् = असत्य>
    वर्ग Action_आधारः VL_NOT_FINAL : सार्वजनिक ActionIfs {
        तार्किक m_notForRerun = असत्य;  // This option is not needed for rerun with --dump-inputs
        तार्किक m_undocumented = असत्य;  // This option is not documented
    सार्वजनिक:
        तार्किक is_मानम्_Needed() ध्रुव override final { प्रत्यागम N_बहुलकम् == en::मानम्_3; }
        तार्किक isFOnOffAllowed() ध्रुव override final { प्रत्यागम N_बहुलकम् == en::FONOFF; }
        तार्किक isNotForRerun() ध्रुव override final { प्रत्यागम m_notForRerun; }
        तार्किक isOnOffAllowed() ध्रुव override final { प्रत्यागम N_बहुलकम् == en::ONOFF; }
        तार्किक is_Partial_मेलनम्_Allowed() ध्रुव override final { प्रत्यागम N_Allow_Partial_मेलनम्; }
        तार्किक isUndocumented() ध्रुव override { प्रत्यागम m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = सत्य;
            प्रत्यागम *एतत्;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = सत्य;
            प्रत्यागम *एतत्;
        }
    };

    // Actual action classes
    प्रतिरूपम् <प्रकारनाम T>
    वर्ग Action_समुच्चयः;  // "-opt" for bool-ish, "-opt val" for int and string
    प्रतिरूपम् <प्रकारनाम N_Bool>
    वर्ग ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    प्रतिरूपम् <प्रकारनाम N_Bool>
    वर्ग ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    वर्ग ActionCbCall;  // Callback without argument for "-opt"
    वर्ग ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    वर्ग ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    प्रतिरूपम् <प्रकारनाम T>
    वर्ग ActionCbVal;  // Callback for "-opt val"
    वर्ग Action_Cb_Partial_मेलनम्;  // Callback "-O3" for "-O"
    वर्ग Action_Cb_Partial_मेलनम्_Val;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<ध्रुव string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    तार्किक m_isFinalized{असत्य};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#परिभाषय V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    प्रतिरूपम् <> \
    वर्ग V3OptionParser::Impl::className<type> final : सार्वजनिक Action_आधारः<enType> { \
        type* ध्रुव m_valp; /* Pointer to an option variable*/ \
\
    सार्वजनिक: \
        स्पष्ट className(type* valp) \
            : m_valp(valp) {} \
        शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(Action_समुच्चयः, तार्किक, *m_valp = सत्य, en::NONE);
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(Action_समुच्चयः, VOptionBool, m_valp->समुच्चयः_True_Or_False(सत्य), en::NONE);
#यदि_अन्तः
V3OPTION_PARSER_DEF_ACT_CLASS(Action_समुच्चयः, पूर्णाङ्क, *m_valp = std::atoi(argp), en::मानम्_3);
V3OPTION_PARSER_DEF_ACT_CLASS(Action_समुच्चयः, string, *m_valp = argp, en::मानम्_3);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, तार्किक, *m_valp = !has_उपसर्गः_FNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, तार्किक, *m_valp = !has_उपसर्गः_No(optp), en::ONOFF);
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->समुच्चयः_True_Or_False(!has_उपसर्गः_No(optp)),
                              en::ONOFF);
#यदि_अन्तः
#अपरिभाषय V3OPTION_PARSER_DEF_ACT_CLASS

#परिभाषय V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    वर्ग V3OptionParser::Impl::className final : सार्वजनिक Action_आधारः<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    सार्वजनिक: \
        उपयुज्य CbType = std::function<funcType>; \
        स्पष्ट className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, शून्य(शून्य), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, शून्य(तार्किक), m_cb(!has_उपसर्गः_FNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, शून्य(तार्किक), m_cb(!has_उपसर्गः_No(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_मेलनम्, शून्य(ध्रुव वर्ण*), m_cb(optp), en::NONE,
                                 सत्य);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(Action_Cb_Partial_मेलनम्_Val, शून्य(ध्रुव वर्ण*, ध्रुव वर्ण*),
                                 m_cb(optp, argp), en::मानम्_3, सत्य);

प्रतिरूपम् <>
वर्ग V3OptionParser::Impl::ActionCbVal<पूर्णाङ्क> final : सार्वजनिक Action_आधारः<en::मानम्_3> {
    std::function<शून्य(पूर्णाङ्क)> m_cb;  // Callback function

सार्वजनिक:
    उपयुज्य CbType = std::function<शून्य(पूर्णाङ्क)>;
    स्पष्ट ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { m_cb(std::atoi(argp)); }
};

प्रतिरूपम् <>
वर्ग V3OptionParser::Impl::ActionCbVal<ध्रुव वर्ण*> final : सार्वजनिक Action_आधारः<en::मानम्_3> {
    std::function<शून्य(ध्रुव वर्ण*)> m_cb;

सार्वजनिक:
    उपयुज्य CbType = std::function<शून्य(ध्रुव वर्ण*)>;
    स्पष्ट ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) override { m_cb(argp); }
};

#अपरिभाषय V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(ध्रुव वर्ण* optp) {
    ध्रुव स्वचालित it = m_pimpl->m_options.find(optp);
    यदि (it != m_pimpl->m_options.अन्तः_2()) प्रत्यागम it->second.get();  // Exact match
    प्रति (स्वचालित&& act : m_pimpl->m_options) {
        यदि (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            यदि (ध्रुव वर्ण* ध्रुव nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::वाक्यदैर्घ्यम्("-fno-")) : शून्यसूचकः) {
                यदि (act.first.substr(std::वाक्यदैर्घ्यम्("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    प्रत्यागम act.second.get();
                }
            }
        }
        यदि (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            यदि (ध्रुव वर्ण* ध्रुव nop
                = VString::startsWith(optp, "-no") ? (optp + std::वाक्यदैर्घ्यम्("-no")) : शून्यसूचकः) {
                यदि (act.first == nop || act.first == ("-"s + nop)) प्रत्यागम act.second.get();
            }
        } अन्यथा यदि (act.second->is_Partial_मेलनम्_Allowed()) {
            यदि (VString::startsWith(optp, act.first)) प्रत्यागम act.second.get();
        }
    }
    प्रत्यागम शून्यसूचकः;
}

प्रतिरूपम् <प्रकारनाम T_Act, प्रकारनाम T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::सङ्कलय(ध्रुव std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{नूतन T_Act{std::move(arg)}};
    UASSERT(opt.आकार() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    ध्रुव स्वचालित inserted_परिणाम = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(inserted_परिणाम.second, opt << " is already registered");
    प्रत्यागम *inserted_परिणाम.first->second;
}

तार्किक V3OptionParser::has_उपसर्गः_FNo(ध्रुव वर्ण* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    यदि (strp[1] == '-') ++strp;
    प्रत्यागम VString::startsWith(strp, "-fno");
}

तार्किक V3OptionParser::has_उपसर्गः_No(ध्रुव वर्ण* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    यदि (strp[1] == '-') ++strp;
    प्रत्यागम VString::startsWith(strp, "-no");
}

std::pair<पूर्णाङ्क, तार्किक> V3OptionParser::parse(पूर्णाङ्क idx, पूर्णाङ्क argc, वर्ण* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    ध्रुव वर्ण* optp = argv[idx];
    यदि (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* ध्रुव actp = find(optp);
    यदि (!actp) प्रत्यागम {0, असत्य};
    यदि (!actp->is_मानम्_Needed()) {
        actp->exec(optp, शून्यसूचकः);
        प्रत्यागम {1, !actp->isNotForRerun()};
    }
    यदि (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        प्रत्यागम {2, !actp->isNotForRerun()};
    }
    प्रत्यागम {0, असत्य};
}

string V3OptionParser::getSuggestion(ध्रुव वर्ण* str) ध्रुव {
    प्रत्यागम m_pimpl->m_spellCheck.best_Candidate_सन्देशः(str);
}

शून्य V3OptionParser::सङ्कलय_Suggestion_Candidate(ध्रुव string& s) {
    m_pimpl->m_spellCheck.प्रक्षिप_Candidate(s);
}

शून्य V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    प्रति (स्वचालित&& opt : m_pimpl->m_options) {
        यदि (opt.second->isUndocumented()) अनुवर्त;
        m_pimpl->m_spellCheck.प्रक्षिप_Candidate(opt.first);
        यदि (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.प्रक्षिप_Candidate(
                "-fno-" + opt.first.substr(std::वाक्यदैर्घ्यम्("-f"), std::string::npos));
        }
        यदि (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.प्रक्षिप_Candidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = सत्य;
}

शून्य V3OptionParser::dumpOptions() {
    प्रति (ध्रुव स्वचालित& opt : m_pimpl->m_spellCheck.candidates()) {
        std::निर्गमः << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{नूतन Impl{}} {}

V3OptionParser::~V3OptionParser() = स्वभाव;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#परिभाषय V3OPTION_PARSER_DEF_OP(act_विधा, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::संक्रियकः()( \
        ध्रुव वर्ण* optp, act_विधा, argType arg) ध्रुव { \
        प्रत्यागम m_parser.सङ्कलय<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(समुच्चयः_2, तार्किक*, Action_समुच्चयः<तार्किक>)
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(समुच्चयः_2, VOptionBool*, Action_समुच्चयः<VOptionBool>)
#यदि_अन्तः
V3OPTION_PARSER_DEF_OP(समुच्चयः_2, पूर्णाङ्क*, Action_समुच्चयः<पूर्णाङ्क>)
V3OPTION_PARSER_DEF_OP(समुच्चयः_2, string*, Action_समुच्चयः<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, तार्किक*, ActionFOnOff<तार्किक>)
V3OPTION_PARSER_DEF_OP(OnOff, तार्किक*, ActionOnOff<तार्किक>)
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#यदि_अन्तः
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<पूर्णाङ्क>::CbType, ActionCbVal<पूर्णाङ्क>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<ध्रुव वर्ण*>::CbType, ActionCbVal<ध्रुव वर्ण*>)
#अपरिभाषय V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::संक्रियकः()(ध्रुव वर्ण* optp, Cb_Partial_मेलनम्,
                                         Impl::Action_Cb_Partial_मेलनम्::CbType cb) ध्रुव {
    ध्रुव आकार_t उपसर्गः_Len = std::वाक्यदैर्घ्यम्(optp);
    ध्रुव स्वचालित wrap = [उपसर्गः_Len, cb](ध्रुव वर्ण* optp) { cb(optp + उपसर्गः_Len); };
    प्रत्यागम m_parser.सङ्कलय<Impl::Action_Cb_Partial_मेलनम्>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::संक्रियकः()(ध्रुव वर्ण* optp, Cb_Partial_मेलनम्_Val,
                                         Impl::Action_Cb_Partial_मेलनम्_Val::CbType cb) ध्रुव {
    ध्रुव आकार_t उपसर्गः_Len = std::वाक्यदैर्घ्यम्(optp);
    ध्रुव स्वचालित wrap
        = [उपसर्गः_Len, cb](ध्रुव वर्ण* optp, ध्रुव वर्ण* argp) { cb(optp + उपसर्गः_Len, argp); };
    प्रत्यागम m_parser.सङ्कलय<Impl::Action_Cb_Partial_मेलनम्_Val>(optp, std::move(wrap));
}
