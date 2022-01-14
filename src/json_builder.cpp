#include "json_builder.h"

#include <stdexcept>
#include <algorithm>
#include <variant>

namespace json{

	// ---------- Builder ---------- //

	KeyItemContext Builder::Key(std::string key){
		if (node_stack_.empty() || !node_stack_.top()->IsMap()) {
			throw std::logic_error("node stack error");
		}

		node_stack_.push(std::make_unique<Node>(std::move(key)));
		return KeyItemContext{*this};
	}

	Builder& Builder::Value(const Node::Value& value){
		NodeGetter node_getter;
		if (is_build_) {
			throw std::logic_error("build error");
		}

		if(node_stack_.empty()){
			is_build_ = true;
			root_ = std::visit(node_getter, value);
		}else if (node_stack_.top()->IsArray()){
			const Array& arr = node_stack_.top()->AsArray();
			const_cast<Array&>(arr).push_back(std::visit(node_getter, value));
		}else if (node_stack_.top()->IsString()){
			std::string str = node_stack_.top()->AsString();
			node_stack_.pop();
			const Dict& dict = node_stack_.top()->AsMap();
			const_cast<Dict&>(dict).emplace(str, std::visit(node_getter, value));
		}else{
			throw std::logic_error("not a node value"); // not a Node::Value
		}

		return *this;
	}

	DictItemContext Builder::StartDict(){
		node_stack_.push(std::make_unique<Node>(Dict{}));

		return DictItemContext(*this);
	}

	ArrayItemContext Builder::StartArray(){
		node_stack_.push(std::make_unique<Node>(Array{}));

		return ArrayItemContext{*this};
	}

	Builder& Builder::EndDict(){
		if (node_stack_.empty() || !node_stack_.top()->IsMap()){
			throw std::logic_error("map error");
		}

		Node node = *node_stack_.top();
		node_stack_.pop();

		return Value(node.AsMap());
	}

	Builder& Builder::EndArray(){
		if (node_stack_.empty() || !node_stack_.top()->IsArray()){
			throw std::logic_error("array error");
		}

		Node node = *node_stack_.top();
		node_stack_.pop();

		return Value(node.AsArray());
	}

	Node Builder::Build(){
		if (!is_build_ || !node_stack_.empty()){
			throw std::logic_error("build error");
		}

		return root_;
	}

	bool Builder::IsBuild() const{
		return is_build_;
	}

	void Builder::SetIsBuild(bool flag){
		is_build_ = flag;
	}

	std::stack<std::unique_ptr<Node>>& Builder::GetNodeStack() {
		return node_stack_;
	}

	Node& Builder::GetRoot(){
		return root_;
	}

    // ---------- KeyItemContext ---------- //

	ValueItemContext KeyItemContext::Value(const Node::Value& value){
		NodeGetter node_getter;

		if (builder_.IsBuild()){
			throw std::logic_error("build error");
		}

		auto& node_stack_ = builder_.GetNodeStack();
		if (node_stack_.empty()){
			builder_.SetIsBuild(true);
			Node& root_ = builder_.GetRoot();
			root_ = std::visit(node_getter, value);
		}else if (node_stack_.top()->IsString()){
			std::string str = node_stack_.top()->AsString();
			node_stack_.pop();
			const Dict& dict = node_stack_.top()->AsMap();
			const_cast<Dict&>(dict).emplace(str, std::visit(node_getter, value));
		}else{
			throw std::logic_error("not a node value after key");
		}

		return ValueItemContext(this->builder_);
	}

    DictItemContext KeyItemContext::StartDict(){
    	builder_.GetNodeStack().push(std::make_unique<Node>(Dict{}));
        return DictItemContext{this->builder_};
    }

    ArrayItemContext KeyItemContext::StartArray(){
    	builder_.GetNodeStack().push(std::make_unique<Node>(Array{}));
        return ArrayItemContext{builder_};
    }

    // ---------- ValueItemContext ---------- //

    KeyItemContext ValueItemContext::Key(std::string key){
    	auto& node_stack_ = builder_.GetNodeStack();
        if (node_stack_.empty() || !node_stack_.top()->IsMap()){
            throw std::logic_error("node stack error");
        }

        node_stack_.push(std::make_unique<Node>(std::move(key)));
        return KeyItemContext{this->builder_};
    }

    Builder& ValueItemContext::EndDict(){
    	auto& node_stack_ = builder_.GetNodeStack();
        if (node_stack_.empty() || !node_stack_.top()->IsMap()){
            throw std::logic_error("map error");
        }

        Node node = *node_stack_.top();
        node_stack_.pop();

        return builder_.Value(node.AsMap());
    }

    // ---------- DictItemContext ---------- //

    KeyItemContext DictItemContext::Key(std::string key){
    	auto& node_stack_ = builder_.GetNodeStack();
    	if (node_stack_.empty() || !node_stack_.top()->IsMap()){
    		throw std::logic_error("node stack error");
    	}

    	node_stack_.push(std::make_unique<Node>(std::move(key)));
    	return KeyItemContext{builder_};
    }

    Builder& DictItemContext::EndDict(){
    	auto& node_stack_ = builder_.GetNodeStack();
    	if (node_stack_.empty() || !node_stack_.top()->IsMap()){
    		throw std::logic_error("map error");
    	}

    	Node node = *node_stack_.top();
    	node_stack_.pop();
    	return builder_.Value(node.AsMap());
    }

    // ---------- ArrayItemContext ---------- //

    ArrayItemContext ArrayItemContext::Value(const Node::Value& value){
    	NodeGetter node_getter;
    	if (builder_.IsBuild()){
    		throw std::logic_error("build error");
    	}

    	auto& node_stack_ = builder_.GetNodeStack();
    	if(node_stack_.empty()){
    		builder_.SetIsBuild(true);
    		auto& root_ = builder_.GetRoot();
    		root_ = std::visit(node_getter, value);
    	}else if (node_stack_.top()->IsArray()){
    		const Array& arr = node_stack_.top()->AsArray();
    		const_cast<Array&>(arr).push_back(std::visit(node_getter, value));
    	}else{
    		throw std::logic_error("not a node value");
    	}

    	return *this;
    }

    DictItemContext ArrayItemContext::StartDict(){
    	builder_.GetNodeStack().push(std::make_unique<Node>(Dict{}));
    	return DictItemContext(builder_);
    }

    ArrayItemContext ArrayItemContext::StartArray(){
    	builder_.GetNodeStack().push(std::make_unique<Node>(Array{}));
    	return *this;
    }

    Builder& ArrayItemContext::EndArray(){
    	auto& node_stack_ = builder_.GetNodeStack();
        if (node_stack_.empty() || !node_stack_.top()->IsArray()) {
            throw std::logic_error("array error");
        }

        Node node = *node_stack_.top();
        node_stack_.pop();
        return builder_.Value(node.AsArray());
    }
}
