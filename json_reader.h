#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"
#include "serialization.h"
#include "request_handler.h"

namespace reader{

	class JsonReader{
	public:
		JsonReader(json::Document json_document)
			: input_(json_document){}

		const json::Node& GetBaseRequest();
		const json::Node& GetStatRequest();
		const json::Node& GetRenderSettings();
		const json::Node& GetRoutingSettings();
		const json::Node& GetSerializationSettings();
		void FillCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue);
		void FillRouter(const transport_catalogue::TransportCatalogue& db_,
				transport_catalogue::TransportRouter& router_);
	private:
		json::Document input_;
	};

	void SaveBase(const transport_catalogue::TransportCatalogue& transport_catalogue,
			const transport_catalogue::renderer::MapRenderer& map_renderer,
			const transport_catalogue::TransportRouter& transport_router,
			const json::Node& serialization_settings);
	void MakeBase(transport_catalogue::TransportCatalogue& transport_catalogue, std::istream& in_json);
    serialize::TransportCatalogue LoadBase(const json::Node& serialization_settings);
    void ProcessRequests(std::istream& in, std::ostream& out);
}
