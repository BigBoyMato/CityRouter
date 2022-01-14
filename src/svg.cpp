#include "svg.h"

#include <iomanip>

namespace svg{

	using namespace std::literals;

	std::ostream& operator<< (std::ostream& os, StrokeLineCap stroke_line_cap) {
		switch(stroke_line_cap)
		{
			case StrokeLineCap::BUTT: os << "butt"sv;
			break;
			case StrokeLineCap::ROUND: os << "round"sv;
			break;
			case StrokeLineCap::SQUARE: os << "square"sv;
			break;
		}
		return os;
	}

	std::ostream& operator<< (std::ostream& os, StrokeLineJoin stroke_line_join) {
		switch(stroke_line_join)
		{
			case StrokeLineJoin::ARCS: os << "arcs"sv;
			break;
			case StrokeLineJoin::BEVEL: os << "bevel"sv;
			break;
			case StrokeLineJoin::MITER: os << "miter"sv;
			break;
			case StrokeLineJoin::MITER_CLIP: os << "miter-clip"sv;
			break;
			case StrokeLineJoin::ROUND: os << "round"sv;
			break;
		}
		return os;
	}

	std::ostream& operator<< (std::ostream& os, const Rgb& rgb) {
		os << "rgb("sv << unsigned(rgb.red) << ","sv << unsigned(rgb.green) << ","sv << unsigned(rgb.blue) << ")"sv;
		return os;
	}

	std::ostream& operator<< (std::ostream& os, const Rgba& rgba) {
		os << "rgba("sv << unsigned(rgba.red) << ","sv << unsigned(rgba.green) << ","sv << unsigned(rgba.blue)
			<< ","sv << rgba.opacity << ")"sv;
		return os;
	}

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		RenderObject(context);

		context.out << std::endl;
	}

	// ---------- Circle ------------------

	Circle& Circle::SetCenter(Point center)  {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius)  {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\""sv;
		RenderAttrs(context.out);
		out << "/>"sv;
	}

	// ---------- Polyline ------------------

	Polyline& Polyline::AddPoint(Point point){
		points_.push_back(point);
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<polyline points=\""sv;
		for (auto it = points_.begin(); it != points_.end(); ++it){
			if (it != points_.begin()){
				out << " "sv;
			}
			out << it->x << ","sv << it->y;
		}
		out << "\""sv;
		RenderAttrs(context.out);
		out << "/>"sv;
	}

	// ---------- Text ------------------

	Text& Text::SetPosition(Point pos){
		position_ = pos;
		return *this;
	}

	Text& Text::SetOffset(Point offset){
		offset_ = offset;
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size){
		size_ = size;
		return *this;
	}

	Text& Text::SetFontFamily(std::string font_family){
		font_family_ = font_family;
		return *this;
	}

	Text& Text::SetFontWeight(std::string font_weight){
		font_weight_ = font_weight;
		return *this;
	}

	Text& Text::SetData(std::string data){
        std::string str = ""s;

        for (const char c : data) {
            switch (c) {
            case '\"':
                str += "&quot;"s; break;
            case '\'':
                str += "&apos;"s; break;
            case '<':
                str += "&lt;"s; break;
            case '>':
                str += "&gt;"s; break;
            case '&':
                str += "&amp;"s; break;
            default:
                str += c; break;
            }
        }

        data_ = std::move(str);
        return *this;
	}

	void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\""sv << position_.x << "\" "sv;
        out << "y=\""sv << position_.y << "\" "sv;
        out << "dx=\""sv << offset_.x << "\" "sv;
        out << "dy=\""sv << offset_.y << "\" "sv;
        out << "font-size=\""sv << size_ << "\" "sv;
        if (!font_family_.empty()) {
            out << "font-family=\""sv << font_family_ << "\""sv;
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        out << ">"sv;
        if (!data_.empty()) {
            out << data_;
        }
        out << "</text>"sv;
	}

	// ---------- Document ------------------

	void Document::AddPtr(std::unique_ptr<Object>&& obj){
		objects_.push_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const{
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;

		for (const auto& object : objects_){
			object.get()->Render({out, 2, 2});
		}
		out << "</svg>"sv;
	}
}
