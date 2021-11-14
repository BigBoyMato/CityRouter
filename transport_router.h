#pragma once
#include "json.h"
#include "router.h"
#include "transport_catalogue.h"

#include <variant>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace transport_catalogue{

	namespace detail{
		struct RouteItemWait{
			std::string stop_name;
			std::chrono::duration<double> time;
		};

		struct RouteItemBus{
			std::string bus_name;
			int span_count;
			std::chrono::duration<double> time;
		};

		struct RouteItem{
			std::variant<RouteItemWait, RouteItemBus> item;
		};

		struct RouteInfo{
			double total_time = 0.0;
			std::vector<RouteItem> items_;
		};

		struct Vertexes{
			size_t start_wait;
			size_t end_wait;
		};

		struct EdgeInfo{
			graph::Edge<double> edge;
			std::string name;
			int span_count = -1;
			std::chrono::duration<double> time{0.0};
		};
	}

	class TransportRouter{
	public:
        using Graph = graph::DirectedWeightedGraph<double>;
        using GraphRouter = graph::Router<double>;

        TransportRouter(const json::Node& routing_settings);

		// default settings
		struct Settings{
			int bus_wait_time_ = 6;
			double bus_velocity_ = 40.0;
		};

		std::optional<detail::RouteInfo> GetRouteInfo(const std::string& stop_name_from, const std::string& stop_name_to) const;
		void AddStop(const std::string& stop_name);
		void AddWaitEdge(const std::string& stop_name);
		void AddBusEdge(const std::string& stop_name_from, const std::string& stop_name_to,
				const std::string& bus_name, const int span_count, const int dist);
		void Build();
		void BuildGraph();
		void BuildRouter();
        void SetEdges(const std::vector<detail::EdgeInfo>& edges);
        void SetVertexes(const std::unordered_map<std::string, detail::Vertexes, std::hash<std::string_view>>& stop_to_vertex_id);
        void SetGraph(graph::DirectedWeightedGraph<double> graph);
        std::unordered_map<std::string, detail::Vertexes, std::hash<std::string_view>> GetStopToVertexId() const;
        Settings GetRoutingSettings() const;
        Graph GetGraph() const;
        std::vector<detail::EdgeInfo> GetEdges() const;

	private:
		Settings settings_;
		std::optional<Graph> graph_ = std::nullopt;
		std::optional<GraphRouter> router_ = std::nullopt;
		std::unordered_map<std::string, detail::Vertexes, std::hash<std::string_view>> stop_to_vertex_id_;
		std::vector<detail::EdgeInfo> edges_;

		void AddEdgesToGraph();
		std::vector<detail::RouteItem> MakeItemsByEdgeIds(const std::vector<graph::EdgeId>& edge_ids) const;
	};
}
