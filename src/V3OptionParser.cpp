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

#include "V3Global.h"
#include "V3Options.h"
#endif
#include "V3Error.h"
#include "V3OptionParser.h"
#include "V3String.h"

//######################################################################
// V3OptionParser::Impl
struct V3OptionParser::Impl final {
    // TYPES

    // Setting for isOnOffAllowed() and isPartialMatchAllowed()
    enum class en : uint8_t {
        NONE,  // "-opt"
        FONOFF,  // "-fopt" and "-fno-opt"
        ONOFF,  // "-opt" and "-no-opt"
        VALUE  // "-opt val"
    };
    // Base class of actual action classes
    template <en N_Mode, bool N_Allow_Partial_Match = false>
    class ActionBase VL_NOT_FINAL : public ActionIfs {
        bool m_notForRerun = false;  // This option is not needed for rerun with --dump-inputs
        bool m_undocumented = false;  // This option is not documented
    public:
        bool isValueNeeded() const override final { 返回 N_Mode == en::VALUE; }
        bool isFOnOffAllowed() const override final { 返回 N_Mode == en::FONOFF; }
        bool isNotForRerun() const override final { 返回 m_notForRerun; }
        bool isOnOffAllowed() const override final { 返回 N_Mode == en::ONOFF; }
        bool isPartialMatchAllowed() const override final { 返回 N_Allow_Partial_Match; }
        bool isUndocumented() const override { 返回 m_undocumented; }
        ActionIfs& undocumented() override {
            m_undocumented = true;
            返回 *this;
        }
        ActionIfs& notForRerun() override {
            m_notForRerun = true;
            返回 *this;
        }
    };

    // Actual action classes
    template <typename T>
    class ActionSet;  // "-opt" for bool-ish, "-opt val" for int and string
    template <typename N_Bool>
    class ActionFOnOff;  // "-fopt" and "-fno-opt" for bool-ish
    template <typename N_Bool>
    class ActionOnOff;  // "-opt" and "-no-opt" for bool-ish
    class ActionCbCall;  // Callback without argument for "-opt"
    class ActionCbFOnOff;  // Callback for "-fopt" and "-fno-opt"
    class ActionCbOnOff;  // Callback for "-opt" and "-no-opt"
    template <typename T>
    class ActionCbVal;  // Callback for "-opt val"
    class ActionCbPartialMatch;  // Callback "-O3" for "-O"
    class ActionCbPartialMatchVal;  // Callback "-debugi-V3Options 3" for "-debugi-"

    // MEMBERS
    std::map<const string, std::unique_ptr<ActionIfs>> m_options;  // All actions for option
    bool m_isFinalized{false};  // Becomes after finalize() is called
    VSpellCheck m_spellCheck;  // Suggests closest option when not found
};

//######################################################################
// Action classes in V3OptionParser::Impl

#define V3OPTION_PARSER_DEF_ACT_CLASS(className, type, body, enType) \
    template <> \
    class V3OptionParser::Impl::className<type> final : public ActionBase<enType> { \
        type* const m_valp; /* Pointer to an option variable*/ \
\
    public: \
        explicit className(type* valp) \
            : m_valp(valp) {} \
        void exec(const char* optp, const char* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, bool, *m_valp = true, en::NONE);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, VOptionBool, m_valp->setTrueOrFalse(true), en::NONE);
#endif
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, 整数, *m_valp = std::atoi(argp), en::VALUE);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionSet, string, *m_valp = argp, en::VALUE);

V3OPTION_PARSER_DEF_ACT_CLASS(ActionFOnOff, bool, *m_valp = !hasPrefixFNo(optp), en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, bool, *m_valp = !hasPrefixNo(optp), en::ONOFF);
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_ACT_CLASS(ActionOnOff, VOptionBool, m_valp->setTrueOrFalse(!hasPrefixNo(optp)),
                              en::ONOFF);
#endif
#undef V3OPTION_PARSER_DEF_ACT_CLASS

#define V3OPTION_PARSER_DEF_ACT_CB_CLASS(className, funcType, body, ...) \
    class V3OptionParser::Impl::className final : public ActionBase<__VA_ARGS__> { \
        std::function<funcType> m_cb; /* Callback function */ \
\
    public: \
        using CbType = std::function<funcType>; \
        explicit className(CbType cb) \
            : m_cb(std::move(cb)) {} \
        void exec(const char* optp, const char* argp) override { body; } \
    }

V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbCall, void(void), m_cb(), en::NONE);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbFOnOff, void(bool), m_cb(!hasPrefixFNo(optp)),
                                 en::FONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbOnOff, void(bool), m_cb(!hasPrefixNo(optp)), en::ONOFF);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatch, void(const char*), m_cb(optp), en::NONE,
                                 true);
V3OPTION_PARSER_DEF_ACT_CB_CLASS(ActionCbPartialMatchVal, void(const char*, const char*),
                                 m_cb(optp, argp), en::VALUE, true);

template <>
class V3OptionParser::Impl::ActionCbVal<整数> final : public ActionBase<en::VALUE> {
    std::function<void(整数)> m_cb;  // Callback function

public:
    using CbType = std::function<void(整数)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    void exec(const char* optp, const char* argp) override { m_cb(std::atoi(argp)); }
};

template <>
class V3OptionParser::Impl::ActionCbVal<const char*> final : public ActionBase<en::VALUE> {
    std::function<void(const char*)> m_cb;

public:
    using CbType = std::function<void(const char*)>;
    explicit ActionCbVal(CbType cb)
        : m_cb{std::move(cb)} {}
    void exec(const char* optp, const char* argp) override { m_cb(argp); }
};

#undef V3OPTION_PARSER_DEF_ACT_CB_CLASS

//######################################################################
// Member functions of V3OptionParser

V3OptionParser::ActionIfs* V3OptionParser::find(const char* optp) {
    const auto it = m_pimpl->m_options.find(optp);
    如果 (it != m_pimpl->m_options.end()) 返回 it->second.get();  // Exact match
    为 (auto&& act : m_pimpl->m_options) {
        如果 (act.second->isFOnOffAllowed()) {  // Find starts with "-fno"
            如果 (const char* const nop
                = VString::startsWith(optp, "-fno-") ? (optp + std::strlen("-fno-")) : nullptr) {
                如果 (act.first.substr(std::strlen("-f"), std::string::npos)
                    == nop) {  // [-f]opt = [-fno-]opt
                    返回 act.second.get();
                }
            }
        }
        如果 (act.second->isOnOffAllowed()) {  // Find starts with "-no"
            如果 (const char* const nop
                = VString::startsWith(optp, "-no") ? (optp + std::strlen("-no")) : nullptr) {
                如果 (act.first == nop || act.first == ("-"s + nop)) 返回 act.second.get();
            }
        } 否则 如果 (act.second->isPartialMatchAllowed()) {
            如果 (VString::startsWith(optp, act.first)) 返回 act.second.get();
        }
    }
    返回 nullptr;
}

template <typename T_Act, typename T_Arg>
V3OptionParser::ActionIfs& V3OptionParser::add(const std::string& opt, T_Arg arg) {
    UASSERT(!m_pimpl->m_isFinalized, "Cannot add after finalize() is called");
    std::unique_ptr<T_Act> act{new T_Act{std::move(arg)}};
    UASSERT(opt.size() >= 2, opt << " is too short");
    UASSERT(opt[0] == '-' || opt[0] == '+', opt << " does not start with either '-' or '+'");
    UASSERT(!(opt[0] == '-' && opt[1] == '-'), "Option must have single '-', but " << opt);
    const auto insertedResult = m_pimpl->m_options.emplace(opt, std::move(act));
    UASSERT(insertedResult.second, opt << " is already registered");
    返回 *insertedResult.first->second;
}

bool V3OptionParser::hasPrefixFNo(const char* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    如果 (strp[1] == '-') ++strp;
    返回 VString::startsWith(strp, "-fno");
}

bool V3OptionParser::hasPrefixNo(const char* strp) {
    UASSERT(strp[0] == '-', strp << " does not start with '-'");
    如果 (strp[1] == '-') ++strp;
    返回 VString::startsWith(strp, "-no");
}

std::pair<整数, bool> V3OptionParser::parse(整数 idx, 整数 argc, char* argv[]) {
    UASSERT(m_pimpl->m_isFinalized, "finalize() must be called before parse()");
    const char* optp = argv[idx];
    如果 (optp[0] == '-' && optp[1] == '-') ++optp;
    ActionIfs* const actp = find(optp);
    如果 (!actp) 返回 {0, false};
    如果 (!actp->isValueNeeded()) {
        actp->exec(optp, nullptr);
        返回 {1, !actp->isNotForRerun()};
    }
    如果 (idx + 1 < argc) {
        actp->exec(optp, argv[idx + 1]);
        返回 {2, !actp->isNotForRerun()};
    }
    返回 {0, false};
}

string V3OptionParser::getSuggestion(const char* str) const {
    返回 m_pimpl->m_spellCheck.bestCandidateMsg(str);
}

void V3OptionParser::addSuggestionCandidate(const string& s) {
    m_pimpl->m_spellCheck.pushCandidate(s);
}

void V3OptionParser::finalize() {
    UASSERT(!m_pimpl->m_isFinalized, "finalize() must not be called twice");
    为 (auto&& opt : m_pimpl->m_options) {
        如果 (opt.second->isUndocumented()) continue;
        m_pimpl->m_spellCheck.pushCandidate(opt.first);
        如果 (opt.second->isFOnOffAllowed()) {
            m_pimpl->m_spellCheck.pushCandidate(
                "-fno-" + opt.first.substr(std::strlen("-f"), std::string::npos));
        }
        如果 (opt.second->isOnOffAllowed()) m_pimpl->m_spellCheck.pushCandidate("-no" + opt.first);
    }
    m_pimpl->m_isFinalized = true;
}

void V3OptionParser::dumpOptions() {
    为 (const auto& opt : m_pimpl->m_spellCheck.candidates()) {
        std::cout << "OPTION: \"" + opt + "\"\n";
    }
}

V3OptionParser::V3OptionParser()
    : m_pimpl{new Impl{}} {}

V3OptionParser::~V3OptionParser() = default;

//######################################################################
// Member functions of V3OptionParser::AppendHelper

#define V3OPTION_PARSER_DEF_OP(actKind, argType, actType) \
    V3OptionParser::ActionIfs& V3OptionParser::AppendHelper::operator()( \
        const char* optp, actKind, argType arg) const { \
        返回 m_parser.add<Impl::actType>(optp, arg); \
    }
V3OPTION_PARSER_DEF_OP(Set, bool*, ActionSet<bool>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(Set, VOptionBool*, ActionSet<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(Set, 整数*, ActionSet<整数>)
V3OPTION_PARSER_DEF_OP(Set, string*, ActionSet<string>)
V3OPTION_PARSER_DEF_OP(FOnOff, bool*, ActionFOnOff<bool>)
V3OPTION_PARSER_DEF_OP(OnOff, bool*, ActionOnOff<bool>)
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
V3OPTION_PARSER_DEF_OP(OnOff, VOptionBool*, ActionOnOff<VOptionBool>)
#endif
V3OPTION_PARSER_DEF_OP(CbCall, Impl::ActionCbCall::CbType, ActionCbCall)
V3OPTION_PARSER_DEF_OP(CbFOnOff, Impl::ActionCbFOnOff::CbType, ActionCbFOnOff)
V3OPTION_PARSER_DEF_OP(CbOnOff, Impl::ActionCbOnOff::CbType, ActionCbOnOff)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<整数>::CbType, ActionCbVal<整数>)
V3OPTION_PARSER_DEF_OP(CbVal, Impl::ActionCbVal<const char*>::CbType, ActionCbVal<const char*>)
#undef V3OPTION_PARSER_DEF_OP

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(const char* optp, CbPartialMatch,
                                         Impl::ActionCbPartialMatch::CbType cb) const {
    const size_t prefixLen = std::strlen(optp);
    const auto wrap = [prefixLen, cb](const char* optp) { cb(optp + prefixLen); };
    返回 m_parser.add<Impl::ActionCbPartialMatch>(optp, std::move(wrap));
}

V3OptionParser::ActionIfs&
V3OptionParser::AppendHelper::operator()(const char* optp, CbPartialMatchVal,
                                         Impl::ActionCbPartialMatchVal::CbType cb) const {
    const size_t prefixLen = std::strlen(optp);
    const auto wrap
        = [prefixLen, cb](const char* optp, const char* argp) { cb(optp + prefixLen, argp); };
    返回 m_parser.add<Impl::ActionCbPartialMatchVal>(optp, std::move(wrap));
}
