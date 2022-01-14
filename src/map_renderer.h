#pragma once
#include "svg.h"
#include "json.h"
#include "domain.h"

#include <vector>
#include <string>
#include <map>

namespace transport_catalogue{

	namespace renderer{

		class MapRenderer{
		public:
			MapRenderer(const json::Node& render_settings);
			svg::Document RenderSvgDocument(const std::map<std::string, transport_catalogue::Bus*> buses) const;
            json::Node GetRenderSettings() const;
		private:
			double width_;
			double height_;
			double padding_;
			double stop_radius_;
			double line_width_;
			int bus_label_font_size_;
			svg::Point bus_label_offset_;
			svg::Point stop_label_offset_;
			int stop_label_font_size_;
			svg::Color underlayer_color_;
			double underlayer_width_;
			std::vector<svg::Color> color_palette_;
            json::Node render_settings_;
        };
	}
}

