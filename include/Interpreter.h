﻿// SPDX-FileCopyrightText: 2020-2021 UnionTech Software Technology Co.,Ltd.

#ifndef INC_Unilang_Interpreter_h_
#define INC_Unilang_Interpreter_h_ 1

#include "Context.h" // for pair, lref, stack, vector, pmr, string, shared_ptr,
//	Environment, Context, TermNode, function, YSLib::allocate_shared,
//	YSLib::Logger, YSLib::unique_ptr;
#include "Parser.h" // for ParseResultOf, ByteParser, SourcedByteParser;
#include <algorithm> // for std::find_if;
#include <cstdlib> // for std::getenv;
#include <istream> // for std::istream;
#include <ostream> // for std::ostream;

namespace Unilang
{

class SeparatorPass final
{
private:
	using TermStackEntry = pair<lref<TermNode>, bool>;
	using TermStack = stack<TermStackEntry, vector<TermStackEntry>>;
	struct TransformationSpec;

	TermNode::allocator_type allocator;
	vector<TransformationSpec> transformations;
	mutable TermStack remained{allocator};

public:
	SeparatorPass(TermNode::allocator_type);
	~SeparatorPass();

	ReductionStatus
	operator()(TermNode&) const;

	void
	Transform(TermNode&, bool, TermStack&) const;
};


template<typename _fParse>
using GParsedValue = typename ParseResultOf<_fParse>::value_type;

template<typename _fParse>
using GTokenizer = function<TermNode(const GParsedValue<_fParse>&)>;

using Tokenizer = GTokenizer<ByteParser>;

using SourcedTokenizer = GTokenizer<SourcedByteParser>;


class Interpreter final
{
private:
	struct LeafConverter final
	{
		const Interpreter& Host;

		YB_ATTR_nodiscard TermNode
		operator()(const GParsedValue<ByteParser>& val) const
		{
			return Host.ConvertLeaf(val);
		}
		YB_ATTR_nodiscard TermNode
		operator()(const GParsedValue<SourcedByteParser>& val) const
		{
			return Host.ConvertLeafSourced(val);
		}
	};

	pmr::pool_resource resource{pmr::new_delete_resource()};
	string line{};
	shared_ptr<Environment> p_ground{};

public:
	bool Echo = std::getenv("ECHO");
	TermNode::allocator_type Allocator{&resource};
	Context Root{resource};
	SeparatorPass Preprocess{Allocator};
	Tokenizer ConvertLeaf;
	SourcedTokenizer ConvertLeafSourced;
	shared_ptr<string> CurrentSource{};
	bool UseSourceLocation = !std::getenv("UNILANG_NO_SRCINFO");
	TermNode Term{Allocator};
	Context::ReducerSequence Backtrace{Allocator};

	Interpreter();
	Interpreter(const Interpreter&) = delete;

	void
	Evaluate(TermNode&);

private:
	ReductionStatus
	ExecuteOnce(Context&);

	ReductionStatus
	ExecuteString(string_view, Context&);

public:
	ReductionStatus
	Exit();

	template<typename... _tParams>
	void
	ShareCurrentSource(_tParams&&... args)
	{
		CurrentSource
			= YSLib::allocate_shared<string>(Allocator, yforward(args)...);
	}

	void
	HandleREPLException(std::exception_ptr, YSLib::Logger&);

	YSLib::unique_ptr<std::istream>
	OpenUnique(string);

	TermNode
	Perform(string_view);

private:
	void
	PrepareExecution(Context&);

public:
	static void
	Print(const TermNode&);

	YB_ATTR_nodiscard TermNode
	Read(string_view);

	YB_ATTR_nodiscard TermNode
	ReadFrom(std::streambuf&) const;
	YB_ATTR_nodiscard TermNode
	ReadFrom(std::istream&) const;

	YB_ATTR_nodiscard TermNode
	ReadParserResult(const ByteParser&) const;
	YB_ATTR_nodiscard TermNode
	ReadParserResult(const SourcedByteParser&) const;

	void
	Run();

	void
	RunLine(string_view);

private:
	ReductionStatus
	RunLoop(Context&);

public:
	void
	RunScript(string);

	bool
	SaveGround();

	std::istream&
	WaitForLine();
};


void
DisplayTermValue(std::ostream&, const TermNode&);

void
PrintTermNode(std::ostream&, const TermNode&);

void
WriteTermValue(std::ostream&, const TermNode&);

} // namespace Unilang;

#endif

