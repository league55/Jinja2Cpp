#include "testers.h"
#include "value_visitors.h"

namespace jinja2
{

template<typename F>
struct TesterFactory
{
    static TesterPtr Create(TesterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }

    template<typename ... Args>
    static IsExpression::TesterFactoryFn MakeCreator(Args&& ... args)
    {
        return [args...](TesterParams params) {return std::make_shared<F>(std::move(params), args...);};
    }
};

std::unordered_map<std::string, IsExpression::TesterFactoryFn> s_testers = {
    {"defined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsDefinedMode)},
    {"startsWith", &TesterFactory<testers::StartsWith>::Create},
    {"eq", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"==", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"equalto", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"even", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsEvenMode)},
    {"ge", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe)},
    {">=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe)},
    {"gt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {">", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {"greaterthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {"in", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsInMode)},
    {"iterable", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsIterableMode)},
    {"le", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe)},
    {"<=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe)},
    {"lower", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsLowerMode)},
    {"lt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"<", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"lessthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"mapping", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsMappingMode)},
    {"ne", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe)},
    {"!=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe)},
    {"number", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsNumberMode)},
    {"odd", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsOddMode)},
    {"sequence", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsSequenceMode)},
    {"string", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsStringMode)},
    {"undefined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUndefinedMode)},
    {"upper", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUpperMode)},
};

TesterPtr CreateTester(std::string testerName, CallParams params)
{
    auto p = s_testers.find(testerName);
    if (p == s_testers.end())
        TesterPtr();

    return p->second(std::move(params));
}

namespace testers
{

Comparator::Comparator(TesterParams params, BinaryExpression::Operation op)
    : m_op(op)
{
    ParseParams({{"b", true}}, params);
}

bool Comparator::Test(const InternalValue& baseVal, RenderContext& context)
{
    auto b = GetArgumentValue("b", context);

    auto cmpRes = Apply2<visitors::BinaryMathOperation>(baseVal, b, m_op);
    return ConvertToBool(cmpRes);
}

#if 0
bool Defined::Test(const InternalValue& baseVal, RenderContext& /*context*/)
{
    return boost::get<EmptyValue>(&baseVal) == nullptr;
}
#endif

StartsWith::StartsWith(TesterParams params)
{
    bool parsed = true;
    auto args = helpers::ParseCallParams({{"str", true}}, params, parsed);
    m_stringEval = args["str"];
}

bool StartsWith::Test(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue val = m_stringEval->Evaluate(context);
    std::string baseStr = AsString(baseVal);
    std::string str = AsString(val);
    return baseStr.find(str) == 0;
}

ValueTester::ValueTester(TesterParams params, ValueTester::Mode mode)
    : m_mode(mode)
{
    switch (m_mode)
    {
    case IsDefinedMode:
        break;
    case IsEvenMode:
        break;
    case IsInMode:
        break;
    case IsIterableMode:
        break;
    case IsLowerMode:
        break;
    case IsMappingMode:
        break;
    case IsNumberMode:
        break;
    case IsOddMode:
        break;
    case IsSequenceMode:
        break;
    case IsStringMode:
        break;
    case IsUndefinedMode:
        break;
    case IsUpperMode:
        break;

    }
}

enum class ValueKind
{
    Empty,
    Boolean,
    String,
    Integer,
    Double,
    List,
    Map,
    KVPair,
    Callable
};

struct ValueKindGetter : visitors::BaseVisitor<ValueKind>
{
    using visitors::BaseVisitor<ValueKind>::operator ();

    ValueKind operator()(const EmptyValue&) const
    {
        return ValueKind::Empty;
    }
    ValueKind operator()(bool) const
    {
        return ValueKind::Boolean;
    }
    template<typename CharT>
    ValueKind operator()(const std::basic_string<CharT>&) const
    {
        return ValueKind::String;
    }
    ValueKind operator()(int64_t) const
    {
        return ValueKind::Integer;
    }
    ValueKind operator()(double) const
    {
        return ValueKind::Double;
    }
    ValueKind operator()(const ListAdapter&) const
    {
        return ValueKind::List;
    }
    ValueKind operator()(const MapAdapter&) const
    {
        return ValueKind::Map;
    }
    ValueKind operator()(const KeyValuePair&) const
    {
        return ValueKind::KVPair;
    }
};

bool ValueTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    bool result = false;
    auto valKind = Apply<ValueKindGetter>(baseVal);

    switch (m_mode)
    {
    case IsIterableMode:
        result = valKind == ValueKind::List || valKind == ValueKind::Map;
        break;
    case IsMappingMode:
        result = valKind == ValueKind::KVPair || valKind == ValueKind::Map;
        break;
    case IsNumberMode:
        result = valKind == ValueKind::Integer || valKind == ValueKind::Double;
        break;
    case IsSequenceMode:
        result = valKind == ValueKind::List;
        break;
    case IsStringMode:
        result = valKind == ValueKind::String;
        break;
    case IsDefinedMode:
        result = valKind != ValueKind::Empty;
        break;
    case IsUndefinedMode:
        result = valKind == ValueKind::Empty;
        break;
    case IsInMode:
        break;
    case IsEvenMode:
        break;
    case IsOddMode:
        break;
    case IsLowerMode:
        break;
    case IsUpperMode:
        break;

    }
    return result;
}

}
