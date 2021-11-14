#include "serialization.h"

using namespace std::string_literals;

// transport catalogue serialization
namespace tcs{

    serialize::Stop SerializeStop(const transport_catalogue::Stop* stop){
        serialize::Stop result;
        result.set_name(stop->name);
        result.add_coordinates(stop->coordinates.lat);
        result.add_coordinates(stop->coordinates.lng);

        return result;
    }

    serialize::Bus SerializeBus(const transport_catalogue::Bus* bus){
        serialize::Bus result;
        result.set_name(bus->name);
        for (const auto& stop : bus->stops){
            result.add_stop(stop->name);
        }

        result.set_is_round(bus->is_round);
        return result;
    }

    serialize::DistanceBetweenStops SerializeDistance(const std::pair<
            const std::pair<transport_catalogue::Stop*, transport_catalogue::Stop*>, int>&
            distance_pair){
        serialize::DistanceBetweenStops result;
        result.set_from_stop(distance_pair.first.first->name);
        result.set_to_stop(distance_pair.first.second->name);
        result.set_distance(distance_pair.second);

        return result;
    }

    serialize::Point SerializePoint(const json::Array& p){
        serialize::Point result;
        result.set_x(p[0].AsDouble());
        result.set_y(p[1].AsDouble());

        return result;
    }

    serialize::Color SerializeColor(const json::Node& node){
        serialize::Color result;
        if (node.IsArray()){
            const json::Array& arr = node.AsArray();
            if (arr.size() == 3){
                serialize::RGB rgb;
                rgb.set_red(arr[0].AsInt());
                rgb.set_green(arr[1].AsInt());
                rgb.set_blue(arr[2].AsInt());
                *result.mutable_rgb() = rgb;
            }else if (arr.size() == 4){
                serialize::RGBA rgba;
                rgba.set_red(arr[0].AsInt());
                rgba.set_green(arr[1].AsInt());
                rgba.set_blue(arr[2].AsInt());
                rgba.set_opacity(arr[3].AsDouble());
                *result.mutable_rgba() = rgba;
            }
        }else if (node.IsString()){
            result.set_name(node.AsString());
        }

        return result;
    }

    serialize::RenderSettings SerializeRenderSettings(const json::Node& render_settings){
        const json::Dict& rs_map = render_settings.AsMap();
        serialize::RenderSettings result;

        result.set_width(rs_map.at("width"s).AsDouble());
        result.set_height(rs_map.at("height"s).AsDouble());
        result.set_padding(rs_map.at("padding"s).AsDouble());
        result.set_stop_radius(rs_map.at("stop_radius"s).AsDouble());
        result.set_line_width(rs_map.at("line_width"s).AsDouble());
        result.set_bus_label_font_size(rs_map.at("bus_label_font_size"s).AsInt());
        *result.mutable_bus_label_offset() = SerializePoint(rs_map.at("bus_label_offset"s).AsArray());
        result.set_stop_label_font_size(rs_map.at("stop_label_font_size"s).AsInt());
        *result.mutable_stop_label_offset() = SerializePoint(rs_map.at("stop_label_offset"s).AsArray());
        *result.mutable_underlayer_color() = SerializeColor(rs_map.at("underlayer_color"s));
        result.set_underlayer_width(rs_map.at("underlayer_width"s).AsDouble());
        for (const auto& c : rs_map.at("color_palette"s).AsArray()){
            *result.add_color_palette() = SerializeColor(c);
        }

        return result;
    }

    serialize::RouterSettings SerializeRoutingSettings(const
        transport_catalogue::TransportRouter::Settings& routing_settings){
        serialize::RouterSettings result;
        result.set_bus_wait_time(routing_settings.bus_wait_time_);
        result.set_bus_velocity(routing_settings.bus_velocity_);

        return result;
    }

    serialize::Graph SerializeGraph(const transport_catalogue::TransportRouter::Graph& graph){
        serialize::Graph result;
        size_t vertex_count = graph.GetVertexCount();
        size_t edge_count = graph.GetEdgeCount();

        for (size_t i = 0; i < edge_count; ++i){
            const graph::Edge<double>& edge = graph.GetEdge(i);
            serialize::Edge s_edge;
            s_edge.set_from(edge.from);
            s_edge.set_to(edge.to);
            s_edge.set_weight(edge.weight);
            *result.add_edge() = s_edge;
        }
        result.set_vertex_count(vertex_count);

        return result;
    }

    serialize::Edge SerializeEdge(const graph::Edge<double>& edge){
        serialize::Edge result;
        result.set_from(edge.from);
        result.set_to(edge.to);
        result.set_weight(edge.weight);

        return result;
    }

    serialize::EdgeInfo SerializeEdgeInfo(const transport_catalogue::detail::EdgeInfo& edge_info){
        serialize::EdgeInfo result;
        result.set_name(edge_info.name);
        *result.mutable_edge() = SerializeEdge(edge_info.edge);
        result.set_span_count(edge_info.span_count);
        result.set_time(std::chrono::duration<double>(edge_info.time).count());

        return result;
    }

    serialize::Vertexes SerializeVertexes(const transport_catalogue::detail::Vertexes& vertexes){
        serialize::Vertexes result;
        result.set_start_wait(vertexes.start_wait);
        result.set_end_wait(vertexes.end_wait);

        return result;
    }

    serialize::Router SerializeRouter(const transport_catalogue::TransportRouter& router){
        serialize::Router result;
        *result.mutable_router_settings() = SerializeRoutingSettings(router.GetRoutingSettings());
        *result.mutable_graph() = SerializeGraph(router.GetGraph());
        for (const auto& edge : router.GetEdges()){
            *result.add_edges() =  SerializeEdgeInfo(edge);
        }
        for(const auto& [name, vertexes] : router.GetStopToVertexId()){
            *result.add_vertexes() = SerializeVertexes(vertexes);
        }

        return result;
    }

	void Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue,
			const transport_catalogue::renderer::MapRenderer& renderer,
			const transport_catalogue::TransportRouter& router,
			std::ostream& output){

		serialize::Catalogue catalogue;
        serialize::RenderSettings render_settings;
        serialize::TransportCatalogue database;

		for (const auto& [name, stop] : transport_catalogue.GetStopsByNames()){
			*catalogue.add_stop() = SerializeStop(stop);
		}

		for (const auto& [name, bus] : transport_catalogue.GetRoutes()){
			*catalogue.add_bus() = SerializeBus(bus);
		}

        for (const auto& distance_pair : transport_catalogue.GetDistances()){
            *catalogue.add_distance() = SerializeDistance(distance_pair);
        }

        *database.mutable_catalogue() = catalogue;
        renderer.GetRenderSettings();
		*database.mutable_render_settings() = SerializeRenderSettings(renderer.GetRenderSettings());
		*database.mutable_router() = SerializeRouter(router);
		database.SerializeToOstream(&output);
	}

    void DeserializeStops(const serialize::TransportCatalogue& database,
                          transport_catalogue::TransportCatalogue& transport_catalogue){
        for (const serialize::Stop& stop : database.catalogue().stop()){
            transport_catalogue.AddStop({stop.name(),
                                         {stop.coordinates(0),
                                          stop.coordinates(1)}});
        }
    }

    void DeserializeDistances(const serialize::TransportCatalogue& database,
                              transport_catalogue::TransportCatalogue& transport_catalogue){
        std::unordered_map<std::pair<std::string, std::string>, int,
                transport_catalogue::detail::StringPairHash> distances;
        for (const serialize::DistanceBetweenStops& dbs : database.catalogue().distance()){
            distances[std::make_pair(dbs.from_stop(), dbs.to_stop())] = dbs.distance();
        }
        transport_catalogue.SetDistances(distances);
    }

    void DeserializeBuses(const serialize::TransportCatalogue& database,
                          transport_catalogue::TransportCatalogue& transport_catalogue){
        for (const serialize::Bus& bus : database.catalogue().bus()){
            std::vector<std::string> stops(bus.stop_size());
            std::move(bus.stop().begin(), bus.stop().end(), stops.begin());

            std::vector<transport_catalogue::Stop*> stop_ptrs;
            stop_ptrs.reserve(stops.size());
            for (const auto& stop : stops){
                const auto found_stop = transport_catalogue.FindStop(stop);
                if (found_stop != nullptr){
                    stop_ptrs.push_back(found_stop);
                }
            }
            transport_catalogue::Bus route;
            route.is_round = bus.is_round();
            route.stops = stop_ptrs;
            route.name = bus.name();

            transport_catalogue.AddRoute(route);
        }
    }

    transport_catalogue::TransportCatalogue Deserialize(const serialize::TransportCatalogue& database){
        transport_catalogue::TransportCatalogue transport_catalogue;
        DeserializeStops(database, transport_catalogue);
        DeserializeDistances(database, transport_catalogue);
        DeserializeBuses(database, transport_catalogue);

        return transport_catalogue;
    }

    json::Node ToNode(const serialize::Point& p){
        return json::Node(json::Array{ {p.x()}, {p.y()} });
    }

    json::Node ToNode(const serialize::Color& c){
        if (!c.name().empty()){
            return json::Node(c.name());
        }else if(c.has_rgb()){
            const serialize::RGB& rgb = c.rgb();
            return json::Node(json::Array{ {rgb.red()}, {rgb.green()}, {rgb.blue()} });
        }else if (c.has_rgba()){
            const serialize::RGBA& rgba = c.rgba();
            return json::Node(json::Array{ {rgba.red()}, {rgba.green()}, {rgba.blue()}, {rgba.opacity()} });
        }else{
            return json::Node("none"s);
        }
    }

    json::Node ToNode(const google::protobuf::RepeatedPtrField<serialize::Color>& cv){
        json::Array result;
        result.reserve(cv.size());
        for (const auto& c : cv){
            result.emplace_back(ToNode(c));
        }
        return json::Node(std::move(result));
    }

    json::Node DeserializeRenderSettings(const serialize::TransportCatalogue& database){
        const serialize::RenderSettings& rs = database.render_settings();
        return json::Node(json::Dict{
                {{"width"s},{ rs.width() }},
                {{"height"s},{ rs.height() }},
                {{"padding"s},{ rs.padding() }},
                {{"stop_radius"s},{ rs.stop_radius() }},
                {{"line_width"s},{ rs.line_width() }},
                {{"bus_label_font_size"s},{ rs.bus_label_font_size() }},
                {{"bus_label_offset"s},ToNode(rs.bus_label_offset())},
                {{"stop_label_font_size"s},{rs.stop_label_font_size()}},
                {{"stop_label_offset"s},ToNode(rs.stop_label_offset())},
                {{"underlayer_color"s},ToNode(rs.underlayer_color())},
                {{"underlayer_width"s},{rs.underlayer_width()}},
                {{"color_palette"s},ToNode(rs.color_palette())},
        });
    }

    graph::Edge<double> DeserializeEdge(const serialize::Edge& edge){
        graph::Edge<double> result;
        result.from = edge.from();
        result.to = edge.to();
        result.weight = edge.weight();

        return result;
    }

    transport_catalogue::TransportRouter::Graph DeserializeGraph(const serialize::Graph graph){
        transport_catalogue::TransportRouter::Graph result(graph.vertex_count());
        for (const auto& edge : graph.edge()){
            result.AddEdge(DeserializeEdge(edge));
        }

        return result;
    }

    transport_catalogue::TransportRouter DeserializeRouter(const serialize::TransportCatalogue& database){
        const serialize::RouterSettings& rs = database.router().router_settings();
        transport_catalogue::TransportRouter router((json::Dict{
                {{"bus_wait_time"s},{ rs.bus_wait_time() }},
                {{"bus_velocity"s},{ rs.bus_velocity() }}
        }));

        return router;
    }
}
