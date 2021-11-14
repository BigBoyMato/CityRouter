    #pragma once

    #include "transport_catalogue.h"
    #include "map_renderer.h"
    #include "transport_router.h"

    #include <transport_catalogue.pb.h>

    // transport catalogue serialization
    namespace tcs{

        void Serialize(const transport_catalogue::TransportCatalogue& transport_catalogue,
                const transport_catalogue::renderer::MapRenderer& renderer,
                const transport_catalogue::TransportRouter& router,
                std::ostream& output);

        serialize::Point SerializePoint(const json::Array& p);
        serialize::Color SerializeColor(const json::Node& node);
        serialize::Stop SerializeStop(const transport_catalogue::Stop* stop);
        serialize::Bus SerializeBus(const transport_catalogue::Bus* bus);
        serialize::RenderSettings SerializeRenderSettings(const json::Node& render_settings);
        serialize::Router SerializeRouter(const transport_catalogue::TransportRouter& router);
        serialize::RouterSettings SerializeRoutingSettings(const
            transport_catalogue::TransportRouter::Settings& routing_settings);
        serialize::Graph SerializeGraph(const transport_catalogue::TransportRouter::Graph& graph);
        serialize::DistanceBetweenStops SerializeDistance(const std::pair<
                const std::pair<transport_catalogue::Stop*, transport_catalogue::Stop*>, int>&
        distance_pair);
        serialize::Vertexes SerializeVertexes(const transport_catalogue::detail::Vertexes& vertexes);
        serialize::Edge SerializeEdge(const graph::Edge<double>& edge);
        serialize::EdgeInfo SerializeEdgeInfo(const transport_catalogue::detail::EdgeInfo& edge_info);


        void DeserializeStops(const serialize::TransportCatalogue& database,
                              transport_catalogue::TransportCatalogue& transport_catalogue);
        void DeserializeDistances(const serialize::TransportCatalogue& database,
                             transport_catalogue::TransportCatalogue& transport_catalogue);
        void DeserializeBuses(const serialize::TransportCatalogue& database,
                         transport_catalogue::TransportCatalogue& transport_catalogue);
        graph::Edge<double> DeserializeEdge(const serialize::Edge& edge);
        transport_catalogue::TransportCatalogue Deserialize(const serialize::TransportCatalogue& database);
        transport_catalogue::TransportRouter DeserializeRouter(const serialize::TransportCatalogue& database);
        json::Node ToNode(const serialize::Point& p);
        json::Node ToNode(const serialize::Color& c);
        json::Node ToNode(const google::protobuf::RepeatedPtrField<serialize::Color>& cv);
        json::Node DeserializeRenderSettings(const serialize::TransportCatalogue& database);
    }
