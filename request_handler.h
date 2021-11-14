#pragma once
#include "transport_catalogue.h"
#include "domain.h"
#include "json.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace transport_catalogue{

	class RequestHandler{
	public:
		RequestHandler(const transport_catalogue::TransportCatalogue& transport_catalogue,
				const renderer::MapRenderer& map_renderer, transport_catalogue::TransportRouter& transport_router);

		void JsonStatRequests(const json::Node& json_document, std::ostream& output);

	private:
		json::Node JsonBuildStopInfo(const json::Dict& request_map, const int& id);
		json::Node JsonBuildBusInfo(const json::Dict& request_map, const int& id);
		json::Node JsonBuildMapInfo(const int& id);
		json::Node JsonBuildRouteInfo(const json::Dict& request_map, const int& id);
		svg::Document RenderMap() const;

		const transport_catalogue::TransportCatalogue& db_;
		const renderer::MapRenderer& renderer_;
        transport_catalogue::TransportRouter& router_;
	};
}
