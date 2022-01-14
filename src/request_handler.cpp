#include "request_handler.h"
#include "json_builder.h"

#include <algorithm>
#include <variant>
#include <sstream>
#include <type_traits>

using namespace std::literals;

namespace transport_catalogue{

	RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& transport_catalogue,
			const renderer::MapRenderer& map_renderer, transport_catalogue::TransportRouter& transport_router)
		: db_(transport_catalogue)
		, renderer_(map_renderer)
		, router_(transport_router)
		{}

	json::Node RequestHandler::JsonBuildStopInfo(const json::Dict& request_map, const int& id){
		json::Builder json_builder;
		json_builder.StartDict().Key("request_id").Value(id);

		const std::string& name = request_map.at("name").AsString();
		const auto found_stop = db_.FindStop(name);

		if (found_stop != nullptr){
			std::vector<std::string> buses;
			for (const transport_catalogue::Bus* bus : db_.GetBusesOnStop(found_stop->name)){
				buses.push_back(bus->name);
			}
			std::sort(buses.begin(), buses.end());

			json_builder.Key("buses").StartArray();
			for (const std::string& bus : buses){
				json_builder.Value(bus);
			}
			json_builder.EndArray();
		}else{
			json_builder.Key("error_message").Value("not found"s);
		}

		return json_builder.EndDict().Build();
	}

	json::Node RequestHandler::JsonBuildBusInfo(const json::Dict& request_map, const int& id){
		json::Builder json_builder;
		json_builder.StartDict().Key("request_id").Value(id);

		const std::string& name = request_map.at("name"s).AsString();
		const auto found_route = db_.FindRoute(name);

		if (found_route != nullptr){
			std::unordered_set<std::string> unique_stops_set;
			for (const auto& stop : found_route->stops){
				unique_stops_set.insert(stop->name);
			}

			json_builder.Key("curvature").Value(found_route->factual_length / found_route->length_by_coordinates);
			json_builder.Key("stop_count").Value(static_cast<int>(found_route->stops.size()));
			json_builder.Key("unique_stop_count").Value(static_cast<int>(unique_stops_set.size()));
			json_builder.Key("route_length").Value(found_route->factual_length);
		}else{
			json_builder.Key("error_message").Value("not found"s);
		}

		return json_builder.EndDict().Build();
	}

	json::Node RequestHandler::JsonBuildMapInfo(const int& id){
		json::Builder json_builder;
		json_builder.StartDict().Key("request_id").Value(id);

		svg::Document map = RenderMap();
		std::ostringstream strm;
		map.Render(strm);
		json_builder.Key("map").Value(strm.str());

		return json_builder.EndDict().Build();
	}

	json::Node RequestHandler::JsonBuildRouteInfo(const json::Dict& request_map, const int& id){
		json::Builder json_builder;
		json_builder.StartDict().Key("request_id").Value(id);

		const auto route_info = router_.GetRouteInfo(request_map.at("from").AsString(), request_map.at("to").AsString());
		if (route_info.has_value()){
			json_builder.Key("items").StartArray();
			for (const auto& elem : route_info.value().items_){
				json_builder.StartDict();
				if (std::holds_alternative<detail::RouteItemWait>(elem.item)){
					const auto route_item_wait = std::get<detail::RouteItemWait>(elem.item);
					json_builder.Key("type").Value("Wait"s);
					json_builder.Key("time").Value(route_item_wait.stop_name);
					json_builder.Key("stop_name").Value(route_item_wait.time.count());
				}else if (std::holds_alternative<detail::RouteItemBus>(elem.item)){
					const auto route_item_bus = std::get<detail::RouteItemBus>(elem.item);
					json_builder.Key("type").Value("Bus"s);
					json_builder.Key("time").Value(route_item_bus.time.count());
					json_builder.Key("span_count").Value(route_item_bus.span_count);
					json_builder.Key("bus").Value(route_item_bus.bus_name);
				}
				json_builder.EndDict();
			}
			json_builder.EndArray();
			json_builder.Key("total_time").Value(route_info.value().total_time);
		}else{
			json_builder.Key("error_message").Value("not found"s);
		}

		return json_builder.EndDict().Build();
	}

	void RequestHandler::JsonStatRequests(const json::Node& json_input, std::ostream& output){
		const json::Array& arr = json_input.AsArray();

		json::Builder json_builder;
		json_builder.StartArray();

		for (const auto& request_node : arr){
			const json::Dict& request_map = request_node.AsMap();
			const int id = request_map.at("id"s).AsInt();
			const std::string& type = request_map.at("type"s).AsString();
			json::Node value;

			if (type == "Stop"sv){
				value = JsonBuildStopInfo(request_map, id);
			}else if(type == "Bus"sv){
				value = JsonBuildBusInfo(request_map, id);
			}else if(type == "Map"sv){
				value = JsonBuildMapInfo(id);
			}else if (type == "Route"sv){
				value = JsonBuildRouteInfo(request_map, id);
			}

			json_builder.Value(value.AsMap());
		}

		json::Print(json::Document{json_builder.EndArray().Build()}, output);
	}

	svg::Document RequestHandler::RenderMap() const{
		return renderer_.RenderSvgDocument(db_.GetSortedBuses());
	}
}
