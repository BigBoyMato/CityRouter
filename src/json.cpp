#include "json.h"

#include <iterator>

using namespace std;

namespace json {

	namespace {
		using namespace std::literals;

		Node LoadNode(istream& input);

        char ReadOneChar(istream& input) {
            if (!input) {
                throw ParsingError("Failed to read from stream"s);
            }
            return static_cast<char>(input.get());
        }

        Node LoadString(istream& input) {
            string line;

            while (input.peek() != '\"') {
                line += ReadOneChar(input);
                if (line.back() == '\\') {
                    const char c = ReadOneChar(input);
                    line.pop_back();
                    switch (c) {
                    case '\"':
                        line += '\"';
                        break;
                    case 'n':
                        line += '\n';
                        break;
                    case 'r':
                        line += '\r';
                        break;
                    case '\\':
                        line += '\\';
                        break;
                    case 't':
                        line += '\t';
                        break;
                    default:
                        throw ParsingError("bad string"s);
                    }
                }
            }
            ReadOneChar(input);

            return Node(move(line));
        }


		std::string LoadLiteral(std::istream& input){
			std::string str;

			while (std::isalpha(input.peek())){
				str.push_back(static_cast<char>(input.get()));
			}

			return str;
		}

		Node LoadArray(istream& input) {
			std::vector<Node> result;

			for (char ch; input >> ch && ch != ']';){
				if (ch != ','){
					input.putback(ch);
				}
				result.push_back(LoadNode(input));
			}
			if (!input){
				throw ParsingError("array parsing error"s);
			}

			return Node(std::move(result));
		}

		Node LoadNumber(std::istream& input){
			std::string parsed_num;

			auto read_char = [&parsed_num, &input] {
				parsed_num += static_cast<char>(input.get());
				if (!input) {
					throw ParsingError("Failed to read number from stream"s);
				}
			};

			auto read_digits = [&input, read_char] {
				if (!std::isdigit(input.peek())) {
					throw ParsingError("A digit is expected"s);
				}
				while (std::isdigit(input.peek())) {
					read_char();
				}
			};

			if (input.peek() == '-') {
				read_char();
			}
			if (input.peek() == '0') {
				read_char();
			} else {
				read_digits();
			}

			bool is_int = true;
			if (input.peek() == '.') {
				read_char();
				read_digits();
				is_int = false;
			}

			if (int ch = input.peek(); ch == 'e' || ch == 'E') {
				read_char();
				if (ch = input.peek(); ch == '+' || ch == '-') {
					read_char();
				}
				read_digits();
				is_int = false;
			}

			try {
				if (is_int) {
					try {
						return std::stoi(parsed_num);
					} catch (...) {
					}
				}
				return std::stod(parsed_num);
			} catch (...) {
				throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
			}
		}

		Node LoadDict(istream& input) {
			Dict dict;

			for (char c; input >> c && c != '}';){
				if (c == '"'){
					std::string key = LoadString(input).AsString();
					if (input >> c && c == ':'){
						if (dict.find(key) != dict.end()){
							throw ParsingError("duplicate key '"s + key + "' have been found");
						}
						dict.emplace(std::move(key), LoadNode(input));
					}else{
						throw ParsingError(": is expected but '"s + c + "' has been found"s);
					}
				}else if (c != ','){
					throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
				}
			}
			if (!input){
				throw ParsingError("Dictionary parsing error"s);
			}

			return Node(std::move(dict));
		}

		Node LoadBool(std::istream& input){
			const auto str = LoadLiteral(input);

			if (str == "true"sv){
				return Node{true};
			}else if (str == "false"sv){
				return Node{false};
			}else{
				throw ParsingError("failed to parse '"s + str + "' as bool"s);
			}
		}

		Node LoadNull(std::istream& input){
			if (auto literal = LoadLiteral(input); literal == "null"sv){
				return Node{nullptr};
			}else{
				throw ParsingError("failed to parse '"s + literal + "' as null"s);
			}
		}

		Node LoadNode(istream& input) {
			char ch;

			if (!(input >> ch)){
				throw ParsingError("unexpected EOF"s);
			}

			switch(ch)
			{
				case '[': return LoadArray(input);
				case '{': return LoadDict(input);
				case '"': return LoadString(input);
				case 't':
				case 'f':
					input.putback(ch);
					return LoadBool(input);
				case 'n':
					input.putback(ch);
					return LoadNull(input);
				default:
					input.putback(ch);
					return LoadNumber(input);
			}
		}

		struct PrintContext{
			std::ostream& out;
			int indent_step = 4;
			int indent = 0;

			void PrintIndent() const{
				for (int i = 0; i < indent; ++i){
					 out.put(' ');
				}
			}

			PrintContext Indented() const{
				return { out, indent_step, indent_step + indent };
			}
		};

	    void PrintString(const std::string& value, std::ostream& out)
	    {
	        out.put('"');
	        for (const char c : value)
	        {
	            switch (c)
	            {
	            case '\r':
	                out << "\\r"sv;
	                break;
	            case '\n':
	                out << "\\n"sv;
	                break;
	            case '"':
	                [[fallthrough]];
	            case '\\':
	                out.put('\\');
	                [[fallthrough]];
	            default:
	                out.put(c);
	                break;
	            }
	        }
	        out.put('"');
	    }

	    void PrintNode(const Node& value, const PrintContext& ctx);

	    template <typename Value> void PrintValue(const Value& value, const PrintContext& ctx){
	        ctx.out << value;
	    }

	    template <> void PrintValue<std::string>(const std::string& value, const PrintContext& ctx){
	        PrintString(value, ctx.out);
	    }

	    template <> void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx){
	        ctx.out << "null"sv;
	    }

	    template <> void PrintValue<bool>(const bool& value, const PrintContext& ctx){
	        ctx.out << (value ? "true"sv : "false"sv);
	    }

	    template <> void PrintValue<Array>(const Array& nodes, const PrintContext& ctx){
	        std::ostream& out = ctx.out;
	        out << "[\n"sv;
	        bool first = true;
	        auto inner_ctx = ctx.Indented();
	        for (const Node& node : nodes)
	        {
	            if (first)
	            {
	                first = false;
	            }
	            else
	            {
	                out << ",\n"sv;
	            }
	            inner_ctx.PrintIndent();
	            PrintNode(node, inner_ctx);
	        }
	        out.put('\n');
	        ctx.PrintIndent();
	        out.put(']');
	    }

	    template <> void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx){
	        std::ostream& out = ctx.out;
	        out << "{\n"sv;
	        bool first = true;
	        auto inner_ctx = ctx.Indented();
	        for (const auto& [key, node] : nodes)
	        {
	            if (first)
	            {
	                first = false;
	            }
	            else
	            {
	                out << ",\n"sv;
	            }
	            inner_ctx.PrintIndent();
	            PrintString(key, ctx.out);
	            out << ": "sv;
	            PrintNode(node, inner_ctx);
	        }
	        out.put('\n');
	        ctx.PrintIndent();
	        out.put('}');
	    }

		void PrintNode(const Node& node, const PrintContext& ctx){
			std::visit([&ctx](const auto& value) { PrintValue(value, ctx); }, node.GetValue());
		}
	}

	Document Load(istream& input){
		return Document{LoadNode(input)};
	}

	void Print(const Document& doc, std::ostream& output){
		PrintNode(doc.GetRoot(), PrintContext{ output });
	}
}
