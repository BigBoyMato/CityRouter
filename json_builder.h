#pragma once
#include "json.h"

#include <stack>
#include <memory>

namespace json{

	class KeyItemContext;
	class ValueItemContext;
	class DictItemContext;
	class ArrayItemContext;

	class Builder{
	public:
		Builder() = default;

		KeyItemContext Key(std::string s);
		Builder& Value(const Node::Value& value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		Builder& EndDict();
		Builder& EndArray();
		Node Build();
		[[nodiscard]] bool IsBuild() const;
		void SetIsBuild(bool flag);
		std::stack<std::unique_ptr<Node>>& GetNodeStack();
		Node& GetRoot();

	private:
		Node root_;
		std::stack<std::unique_ptr<Node>> node_stack_;
		bool is_build_ = false;
	};

	class ContextBase{
	public:
		explicit ContextBase(Builder& builder)
			: builder_(builder)
		{}

	protected:
		Builder& builder_;
	};

	class KeyItemContext : ContextBase{
	public:
		using ContextBase::ContextBase;

		ValueItemContext Value(const Node::Value& value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
	};

	class ValueItemContext : ContextBase{
	public:
		using ContextBase::ContextBase;

		KeyItemContext Key(std::string key);
		Builder& EndDict();
	};

	class DictItemContext : ContextBase{
	public:
		using ContextBase::ContextBase;

		KeyItemContext Key(std::string key);
		Builder& EndDict();
	};

	class ArrayItemContext : ContextBase{
	public:
		using ContextBase::ContextBase;

		ArrayItemContext Value(const Node::Value& value);
		DictItemContext StartDict();
		ArrayItemContext StartArray();
		Builder& EndArray();
	};
}
