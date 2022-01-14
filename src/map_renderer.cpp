#include "map_renderer.h"

#include <vector>
#include <algorithm>


namespace transport_catalogue{

	namespace renderer{

		using namespace std::literals;

		inline const double Epsilon = 1e-6;

		class SphereProjector{
		public:
			template <typename PointInputIt>
			SphereProjector(PointInputIt points_begin, PointInputIt points_end,
						double max_width, double max_height, double padding)
							: padding_ (padding)
						{
							if (points_begin == points_end){
								return;
							}

							const auto [left_it, right_it] = std::minmax_element(
									points_begin, points_end,
									[](auto lhs, auto rhs){
										return lhs.lng < rhs.lng;
									});

							min_lon_ = left_it->lng;

							const double max_lon = right_it->lng;

							const auto [bottom_it, top_it] = std::minmax_element(
									points_begin, points_end,
									[](auto lhs, auto rhs){
										return lhs.lat < rhs.lat;
									});

							const double min_lat = bottom_it->lat;
							max_lat_ = top_it->lat;

							std::optional<double> width_zoom;
							if (!IsZero(max_lon - min_lon_)){
								width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
							}

							std::optional<double> height_zoom;
							if (!IsZero(max_lat_ - min_lat)){
								height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
							}

							if (width_zoom && height_zoom){
								zoom_coeff_ = std::min(*width_zoom, *height_zoom);
							}else if (width_zoom){
								zoom_coeff_ = *width_zoom;
							}else if (height_zoom){
								zoom_coeff_ = *height_zoom;
							}
						}

			svg::Point operator() (geo::Coordinates coordinates) const;

		private:

			bool IsZero(double value){
				return std::abs(value) < Epsilon;
			}

			double padding_;
			double min_lon_ = 0;
			double max_lat_ = 0;
			double zoom_coeff_ = 0;
		};

        json::Node MapRenderer::GetRenderSettings() const{
            return render_settings_;
        }

		MapRenderer::MapRenderer(const json::Node& render_settings)
            : render_settings_(render_settings)
		{
			if (!render_settings.IsNull()){
				const json::Dict& settings_map = render_settings.AsMap();

				width_ = settings_map.at("width"s).AsDouble();
				height_ = settings_map.at("height"s).AsDouble();
				padding_ = settings_map.at("padding"s).AsDouble();
				stop_radius_ = settings_map.at("stop_radius"s).AsDouble();
				line_width_ = settings_map.at("line_width"s).AsDouble();
				bus_label_font_size_ = settings_map.at("bus_label_font_size"s).AsInt();
				bus_label_offset_ = {settings_map.at("bus_label_offset"s).AsArray()[0].AsDouble(),
						settings_map.at("bus_label_offset"s).AsArray()[1].AsDouble()};
				stop_label_font_size_ = settings_map.at("stop_label_font_size"s).AsInt();
				stop_label_offset_ = { settings_map.at("stop_label_offset"s).AsArray()[0].AsDouble(),
						settings_map.at("stop_label_offset"s).AsArray()[1].AsDouble() };

				if (settings_map.at("underlayer_color"s).IsArray()){
					const json::Array& arr = settings_map.at("underlayer_color"s).AsArray();
					if (arr.size() == 3){
						svg::Rgb rgb_colors(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
						underlayer_color_ = rgb_colors;
					}else if (arr.size() == 4){
						svg::Rgba rgba_colors(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
						underlayer_color_ = rgba_colors;
					}else{
						throw std::logic_error("broken array"s);
					}
				}else if(settings_map.at("underlayer_color"s).IsString()){
					underlayer_color_ = settings_map.at("underlayer_color"s).AsString();
				}else{
					throw std::logic_error("color identity error"s);
				}

				underlayer_width_ = settings_map.at("underlayer_width"s).AsDouble();

				const json::Array& color_palette = settings_map.at("color_palette"s).AsArray();

				for (const auto& node : color_palette){
					if (node.IsArray()){
						const json::Array& arr = node.AsArray();

						if (arr.size() == 3){
							svg::Rgb rgb_colors(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt());
							color_palette_.push_back(rgb_colors);
						}else if (arr.size() == 4){
							svg::Rgba rgba_colors(arr[0].AsInt(), arr[1].AsInt(), arr[2].AsInt(), arr[3].AsDouble());
							color_palette_.push_back(rgba_colors);
						}else{
							throw std::logic_error("broken array"s);
						}
					}else if (node.IsString()){
						color_palette_.push_back(node.AsString());
					}else{
						throw std::logic_error("palette color identity error"s);
					}
				}
			}

            render_settings_ = render_settings;
		}

		svg::Document MapRenderer::RenderSvgDocument(const std::map<std::string, transport_catalogue::Bus*> buses) const{
			std::map<std::string, transport_catalogue::Stop*> all_stops;
			std::vector<geo::Coordinates> all_coordinates;
			svg::Document SvgDocument;

			for (const auto& [bus_name, bus_ptr] : buses){
				for (const auto& stop : bus_ptr->stops){
					all_stops[stop->name] = stop;
					all_coordinates.push_back(stop->coordinates);
				}
			}

			SphereProjector sphere_projector(all_coordinates.begin(), all_coordinates.end(),
					width_, height_, padding_);

			unsigned color_num = 0;

			for (const auto& [bus_name, bus_ptr] : buses){
				if (bus_ptr->stops.size() != 0){
					svg::Polyline polyline;
					std::vector<geo::Coordinates> points;

					for (const auto& stop : bus_ptr->stops){
						polyline.AddPoint(sphere_projector(stop->coordinates));
					}

					polyline.SetFillColor("none"s);
					polyline.SetStrokeColor(color_palette_[color_num]);
					polyline.SetStrokeWidth(line_width_);
					polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
					polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

					if (color_num < (color_palette_.size() - 1)){
						++color_num;
					}else{
						color_num = 0;
					}

					SvgDocument.Add(polyline);
				}
			}

			color_num = 0;

			for (const auto& [bus_name, bus_ptr] : buses){
				if (bus_ptr->stops.size() != 0){

					svg::Text text, text_underlayer;
					text_underlayer.SetData(std::string(bus_name));
					text.SetData(std::string(bus_name));
					text.SetFillColor(color_palette_[color_num]);

					if (color_num < (color_palette_.size() - 1)){
						++color_num;
					}else{
						color_num = 0;
					}

					text_underlayer.SetFillColor(underlayer_color_);
					text_underlayer.SetStrokeColor(underlayer_color_);
					text.SetFontFamily("Verdana"s);
					text_underlayer.SetFontFamily("Verdana"s);
					text.SetFontSize(bus_label_font_size_);
					text_underlayer.SetFontSize(bus_label_font_size_);
					text.SetFontWeight("bold"s);
					text_underlayer.SetFontWeight("bold"s);
					text.SetPosition(sphere_projector(bus_ptr->stops[0]->coordinates));
					text_underlayer.SetPosition(sphere_projector(bus_ptr->stops[0]->coordinates));
					text.SetOffset(bus_label_offset_);
					text_underlayer.SetStrokeWidth(underlayer_width_);
					text_underlayer.SetOffset(bus_label_offset_);
					text_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
					text_underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

					SvgDocument.Add(text_underlayer);
					SvgDocument.Add(text);

					if ((!bus_ptr->is_round && bus_ptr->stops.size() > 1
							&& (bus_ptr->stops[0] != bus_ptr->stops[bus_ptr->stops.size() / 2]))
							|| (bus_ptr->is_round && bus_ptr->stops[0]->name
								!= bus_ptr->stops[bus_ptr->stops.size() - 1]->name)){

						svg::Text text_to_add = text;
						svg::Text text_to_add_underlayer = text_underlayer;
						size_t half_route = bus_ptr->stops.size() / 2;

						text_to_add.SetPosition(sphere_projector(bus_ptr->stops[half_route]->coordinates));
						text_to_add_underlayer.SetPosition(sphere_projector(bus_ptr->stops[half_route]->coordinates));

						SvgDocument.Add(text_to_add_underlayer);
						SvgDocument.Add(text_to_add);
					}
				}
			}

			for (const auto& [stop_name, stop_ptr] : all_stops){
				svg::Circle circle;
				circle.SetCenter(sphere_projector(stop_ptr->coordinates));
				circle.SetRadius(stop_radius_);
				circle.SetFillColor("white"s);

				SvgDocument.Add(circle);
			}

			for (const auto& [stop_name, stop_ptr] : all_stops){
				svg::Text text, text_underlayer;

				text.SetPosition(sphere_projector(stop_ptr->coordinates));
				text.SetOffset(stop_label_offset_);
				text.SetFontSize(stop_label_font_size_);
				text.SetFontFamily("Verdana"s);
				text.SetData(stop_ptr->name);
				text.SetFillColor("black"s);

				text_underlayer.SetPosition(sphere_projector(stop_ptr->coordinates));
				text_underlayer.SetOffset(stop_label_offset_);
				text_underlayer.SetFontSize(stop_label_font_size_);
				text_underlayer.SetFontFamily("Verdana"s);
				text_underlayer.SetData(stop_ptr->name);
				text_underlayer.SetFillColor(underlayer_color_);
				text_underlayer.SetStrokeWidth(underlayer_width_);
				text_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
				text_underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
				text_underlayer.SetStrokeColor(underlayer_color_);

				SvgDocument.Add(text_underlayer);
				SvgDocument.Add(text);
			}
			return SvgDocument;
		}

		svg::Point SphereProjector::operator() (geo::Coordinates coords) const{
			return { (coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
		}
	}
}

