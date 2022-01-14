#include "json_reader.h"
#include "graph.h"
#include "serialization.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

using namespace std::literals;

namespace reader{
	const json::Node node;

	const json::Node& JsonReader::GetBaseRequest(){
		if (input_.GetRoot().AsMap().count("base_requests"s)){
			return input_.GetRoot().AsMap().at("base_requests"s);
		}
		return node;
	}

	const json::Node& JsonReader::GetStatRequest(){
		if (input_.GetRoot().AsMap().count("stat_requests"s)){
			return input_.GetRoot().AsMap().at("stat_requests"s);
		}
		return node;
	}

	const json::Node& JsonReader::GetRenderSettings(){
		if (input_.GetRoot().AsMap().count("render_settings"s)){
			return input_.GetRoot().AsMap().at("render_settings"s);
		}
		return node;
	}

	const json::Node& JsonReader::GetRoutingSettings(){
		if (input_.GetRoot().AsMap().count("routing_settings"s)){
			return input_.GetRoot().AsMap().at("routing_settings"s);
		}
		return node;
	}

	const json::Node& JsonReader::GetSerializationSettings(){
		if (input_.GetRoot().AsMap().count("serialization_settings"s)){
			return input_.GetRoot().AsMap().at("serialization_settings"s);
		}
		return node;
	}

	void JsonReader::FillCatalogue(transport_catalogue::TransportCatalogue& transport_catalogue) {

		const json::Array& arr = GetBaseRequest().AsArray();

		std::unordered_map<std::pair<std::string, std::string>, int, transport_catalogue::detail::StringPairHash> distances;
		std::unordered_map<std::string, std::vector<std::string>> bus_to_stops;
		std::unordered_map<std::string, bool> bus_to_circle;

		for (const auto& request_node : arr){
			const json::Dict& request_map = request_node.AsMap();
			const std::string& name = request_map.at("name"s).AsString();

			if (request_map.at("type"s).AsString() == "Stop"s){

				// ADD STOPS
				transport_catalogue.AddStop({name,
					{request_map.at("latitude"s).AsDouble(), request_map.at("longitude"s).AsDouble()}});

				const json::Dict& near_stops = request_map.at("road_distances"s).AsMap();
				for (const auto& [key_stop_name, val_dist_node] : near_stops){
					distances[std::make_pair(name, key_stop_name)] = val_dist_node.AsInt();
				}
			}else if (request_map.at("type"s).AsString() == "Bus"s){
				const json::Array& bus_stops = request_map.at("stops"s).AsArray();
				const size_t stops_count = bus_stops.size();
				const bool is_round = request_map.at("is_roundtrip"s).AsBool();

				bus_to_circle[name] = is_round;

				if (stops_count != 0 ){
					bus_to_stops[name]
								 .reserve(is_round ? stops_count : stops_count * 2);

					for (size_t i = 0; i < bus_stops.size(); ++i){
						bus_to_stops.at(name)
								.emplace_back(bus_stops[i].AsString());

						if ((i == bus_stops.size() - 1) && !is_round){
							for (int j = bus_to_stops.at(name).size() - 2; j >= 0; --j){
								bus_to_stops.at(name)
										.emplace_back(bus_to_stops.at(name)[j]);
							}
						}
					}
				}else{
					bus_to_stops[name] = {};
				}
			}
		}

		// SET DISTANCES
		transport_catalogue.SetDistances(distances);

		// ADD ROUTES
		for (const auto& [name, stop_names] : bus_to_stops){

			// NEEDED FOR MAP_RENDERER
			std::vector<transport_catalogue::Stop*> stop_ptrs;
			stop_ptrs.reserve(stop_names.size());
			for (const auto& stop : stop_names){
				const auto found_stop = transport_catalogue.FindStop(stop);
				if (found_stop != nullptr){
					stop_ptrs.push_back(found_stop);
				}
			}

			transport_catalogue::Bus bus;
			bus.name = name;
			bus.stops = stop_ptrs;
			bus.is_round = bus_to_circle.at(name);
			transport_catalogue.AddRoute(bus);
		}
	}

	void JsonReader::FillRouter(const transport_catalogue::TransportCatalogue& db_,
			transport_catalogue::TransportRouter& router_){
		for (const auto& stop : db_.GetStops()){
			router_.AddStop(stop.name);
			router_.AddWaitEdge(stop.name);
		}
		for (const auto& bus : db_.GetBuses()){
			for (size_t i = 0; i < bus.stops.size() - 1; ++i){
				const auto stop_from = bus.stops[i];
				auto prev_stop = stop_from;

				int prev_actual = 0;
				for (size_t j = i + 1; j < bus.stops.size(); ++j){
					const auto stop_to = bus.stops[j];
					int actual = 0;

					if (db_.GetDistance({prev_stop, stop_to}).has_value()){
						actual = db_.GetDistance({prev_stop, stop_to}).value();
					}else{ // has no value from -> to
						if (db_.GetDistance({stop_to, prev_stop}).has_value()){
							actual = db_.GetDistance({stop_to, prev_stop}).value();
						}
					}

					router_.AddBusEdge(
							stop_from->name,
							stop_to->name,
							bus.name,
							j - i,
							prev_actual + actual
						);
					prev_stop = stop_to;
					prev_actual += actual;
				}
			}
		}

		router_.Build();
	}

	void SaveBase(const transport_catalogue::TransportCatalogue& transport_catalogue,
			const transport_catalogue::renderer::MapRenderer& map_renderer,
			const transport_catalogue::TransportRouter& transport_router,
			const json::Node& serialization_settings){

        std::ofstream out(serialization_settings.AsMap().at("file"s).AsString(), std::ios::binary);
        if (out.is_open()){
			tcs::Serialize(transport_catalogue, map_renderer, transport_router, out);
		}
	}

	void MakeBase(transport_catalogue::TransportCatalogue& transport_catalogue, std::istream& in_json){
		reader::JsonReader input_json(json::Load(in_json));

		// FILL TRANSPORT CATALOGUE
		input_json.FillCatalogue(transport_catalogue);

		// FILL TRANSPORT ROUTER
		transport_catalogue::TransportRouter transport_router(input_json.GetRoutingSettings());
		input_json.FillRouter(transport_catalogue, transport_router);

		// FILL MAP RENDERER
		transport_catalogue::renderer::MapRenderer map_renderer(input_json.GetRenderSettings());

		// SAVE SERIALIZED BASE
		const auto serialization_settings = input_json.GetSerializationSettings();
		SaveBase(transport_catalogue, map_renderer, transport_router, serialization_settings);
	}

    serialize::TransportCatalogue LoadBase(const json::Node& serialization_settings){
        std::ifstream in(serialization_settings.AsMap().at("file"s).AsString(), std::ios::binary);
        serialize::TransportCatalogue database;
        database.ParseFromIstream(&in);
        return database;
    }

    void ProcessRequests(std::istream& in, std::ostream& out){
        reader::JsonReader input_json(json::Load(in));
        serialize::TransportCatalogue database = LoadBase(input_json.GetSerializationSettings());

        const transport_catalogue::TransportCatalogue transport_catalogue = tcs::Deserialize(database);
        const transport_catalogue::renderer::MapRenderer map_renderer = tcs::DeserializeRenderSettings(database);
        transport_catalogue::TransportRouter transport_router = tcs::DeserializeRouter(database);

        input_json.FillRouter(transport_catalogue, transport_router);
        transport_catalogue::RequestHandler request_handler(transport_catalogue, map_renderer, transport_router);
        request_handler.JsonStatRequests(input_json.GetStatRequest(), out);
    }
}

