#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <optional>
#include <vector>
#include <variant>
#include <sstream>

namespace svg{

	struct Rgb{
		Rgb() = default;

		Rgb(uint8_t r, uint8_t g, uint8_t b)
			: red(r)
			, green(g)
			, blue(b)
		{}

		uint8_t red = 0;
		uint8_t	green = 0;
		uint8_t	blue = 0;
	};

	struct Rgba : Rgb{
		Rgba() = default;

		Rgba(uint8_t r, uint8_t g, uint8_t b, double o)
			: Rgb(r, g, b)
			, opacity(o)
		{}
		double opacity = 1.0;
	};

	using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

	inline const Color NoneColor{};

	enum class StrokeLineCap {
		BUTT,
		ROUND,
		SQUARE,
	};

	enum class StrokeLineJoin {
		ARCS,
		BEVEL,
		MITER,
		MITER_CLIP,
		ROUND,
	};

	std::ostream& operator<< (std::ostream& os, StrokeLineCap stroke_line_cap);
	std::ostream& operator<< (std::ostream& os, StrokeLineJoin stroke_line_join);
	std::ostream& operator<< (std::ostream& os, const Rgb& rgb);
	std::ostream& operator<< (std::ostream& os, const Rgba& rgba);

	struct StringColorPrinter {
		std::ostream& os;

		void operator()(std::monostate) const {
			using namespace std::string_literals;
			os << "none"s;
		}
		void operator()(std::string color) const {
			os << color;
		}
		void operator()(Rgb rgb) const {
			os << rgb;
		}
		void operator()(Rgba rgba) const {
			os << rgba;
		}
	};

	struct Point {
		Point() = default;
		Point(double x, double y)
			: x(x)
			, y(y) {
		}
		double x = 0;
		double y = 0;

	};

	struct RenderContext {
		RenderContext(std::ostream& out)
			: out(out) {
		}

		RenderContext(std::ostream& out, int indent_step, int indent = 0)
			: out(out)
			, indent_step(indent_step)
			, indent(indent) {
		}

		RenderContext Indented() const {
			return {out, indent_step, indent + indent_step};
		}

		void RenderIndent() const {
			for (int i = 0; i < indent; ++i) {
				out.put(' ');
			}
		}

		std::ostream& out;
		int indent_step = 0;
		int indent = 0;
	};

	class Object {
	public:
		void Render(const RenderContext& context) const;
		virtual ~Object() = default;
	private:
		virtual void RenderObject(const RenderContext& context) const = 0;
	};

	class ObjectContainer {
	public:
		template <typename Obj>
		void Add(Obj obj){
			AddPtr(std::make_unique<Obj>(std::move(obj)));
		}

		virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
		virtual ~ObjectContainer() = default;
	};

	template <typename Owner>
	class PathProps {
	public:
		Owner& SetFillColor(Color color){
			fill_color_ = std::move(color);
			return AsOwner();
		}

		Owner& SetStrokeColor(Color color) {
			stroke_color_ = std::move(color);
			return AsOwner();
		}

		Owner& SetStrokeWidth(double width) {
			width_ = std::move(width);
			return AsOwner();
		}

		Owner& SetStrokeLineCap(StrokeLineCap line_cap){
			line_cap_ = std::move(line_cap);
			return AsOwner();
		}

		Owner& SetStrokeLineJoin(StrokeLineJoin line_join){
			line_join_ = std::move(line_join);
			return AsOwner();
		}

	protected:
		~PathProps() = default;

		void RenderAttrs(std::ostream& out) const {
			using namespace std::literals;

			if (fill_color_.has_value()) {
				std::ostringstream strm;
				out << " fill=\""sv;
				std::visit(StringColorPrinter{strm}, *fill_color_);
				out << strm.str();
				out << "\""sv;
			}
			if (stroke_color_.has_value()) {
				std::ostringstream strm;
				out << " stroke=\""sv;
				std::visit(StringColorPrinter{strm}, *stroke_color_);
				out << strm.str();
				out << "\""sv;
			}
			if (width_.has_value()){
				out << " stroke-width=\""sv << *width_ << "\""sv;
			}
			if (line_cap_.has_value()){
				out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
			}
			if (line_join_.has_value()){
				out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
			}
		}

		std::optional<Color> fill_color_;
		std::optional<Color> stroke_color_;
		std::optional<double> width_;
		std::optional<StrokeLineCap> line_cap_;
		std::optional<StrokeLineJoin> line_join_;

	private:
		Owner& AsOwner() {
			return static_cast<Owner&>(*this);
		}
	};

	class Circle final : public Object, public PathProps<Circle> {
	public:
		Circle& SetCenter(Point center);
		Circle& SetRadius(double radius);
	private:
		void RenderObject(const RenderContext& context) const override;
		Point center_;
		double radius_ = 1.0;
	};

	class Polyline final : public Object, public PathProps<Polyline> {
	public:
		Polyline& AddPoint(Point point);
	private:
		void RenderObject(const RenderContext& context) const override;
		std::vector<Point> points_;
	};

	class Text final : public Object, public PathProps<Text> {
	public:
		Text& SetPosition(Point pos);
		Text& SetOffset(Point offset);
		Text& SetFontSize(uint32_t size);
		Text& SetFontFamily(std::string font_family);
		Text& SetFontWeight(std::string font_weight);
		Text& SetData(std::string data);
	private:
		void RenderObject(const RenderContext& context) const override;
		Point position_ = {0.0, 0.0};
		Point offset_ = {0.0, 0.0};
		uint32_t size_ = 1;
		std::string font_family_;
		std::string font_weight_;
		std::string data_;
	};

	class Document : public ObjectContainer{
	public:
		Document() = default;

		void AddPtr(std::unique_ptr<Object>&& obj) override;
		void Render(std::ostream& out) const;
	private:
		std::vector<std::unique_ptr<Object>> objects_;
	};
}
