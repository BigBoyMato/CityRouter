#include "transport_router.h"

namespace transport_catalogue{

	TransportRouter::TransportRouter(const json::Node& routing_settings){
		if (!routing_settings.IsNull()){
			const json::Dict& settings_map = routing_settings.AsMap();
			settings_ = {settings_map.at("bus_wait_time").AsInt(), settings_map.at("bus_velocity").AsDouble()};
		}
	}

	std::vector<detail::RouteItem> TransportRouter::MakeItemsByEdgeIds(const std::vector<graph::EdgeId>& edge_ids) const{
		std::vector<detail::RouteItem> items;
		items.reserve(edge_ids.size());
		for (const auto id : edge_ids){
			const detail::EdgeInfo& edge_info = edges_[id];
			detail::RouteItem route_item;
			if (edge_info.span_count == -1){
				detail::RouteItemWait item_wait = {edge_info.name, edge_info.time};
				route_item.item = item_wait;
			}else{
				detail::RouteItemBus item_bus = {edge_info.name, edge_info.span_count, edge_info.time};
				route_item.item = item_bus;
			}
			items.push_back(std::move(route_item));
		}

		return items;
	}

	std::optional<detail::RouteInfo> TransportRouter::GetRouteInfo(const std::string& stop_name_from,
			const std::string& stop_name_to) const{
		const auto stop_from_it = stop_to_vertex_id_.find(stop_name_from);
		const auto stop_to_it = stop_to_vertex_id_.find(stop_name_to);
		if (stop_from_it != stop_to_vertex_id_.end() && stop_to_it != stop_to_vertex_id_.end()){
			const auto route = router_->BuildRoute(stop_from_it->second.start_wait, stop_to_it->second.start_wait);
			if (route){
				return detail::RouteInfo{route->weight, MakeItemsByEdgeIds(route->edges)};
			}
		}
		return std::nullopt;
	}

	void TransportRouter::AddStop(const std::string& stop_name){
		if (!stop_to_vertex_id_.count(stop_name)){
			const size_t sz = stop_to_vertex_id_.size();
			stop_to_vertex_id_[stop_name] = { sz * 2, sz * 2 + 1 };
		}
	}

	void TransportRouter::AddWaitEdge(const std::string& stop_name){
		detail::EdgeInfo edge{
			{
			stop_to_vertex_id_[stop_name].start_wait,
			stop_to_vertex_id_[stop_name].end_wait,
			static_cast<double>(settings_.bus_wait_time_)
			},
			stop_name,
			-1,
			static_cast<std::chrono::duration<double>>(settings_.bus_wait_time_)
		};

		edges_.push_back(std::move(edge));
	}

	void TransportRouter::AddBusEdge(const std::string& stop_name_from, const std::string& stop_name_to,
			const std::string& bus_name, const int span_count, const int dist){
		const double TO_MINUTES = 0.06;
		detail::EdgeInfo edge{
			{
				stop_to_vertex_id_[stop_name_from].end_wait,
				stop_to_vertex_id_[stop_name_to].start_wait,
				dist / settings_.bus_velocity_ * TO_MINUTES
			},
			bus_name,
			span_count,
			static_cast<std::chrono::duration<double>>(dist / settings_.bus_velocity_ * TO_MINUTES)
		};

		edges_.push_back(std::move(edge));
	}

	void TransportRouter::AddEdgesToGraph(){
		for (const auto& edge_info : edges_){
			graph_->AddEdge(edge_info.edge);
		}
	}

	void TransportRouter::BuildGraph(){
		if (!graph_){
			graph_ = std::move(Graph(stop_to_vertex_id_.size() * 2));
		}
		AddEdgesToGraph();
	}

    void TransportRouter::SetEdges(const std::vector<detail::EdgeInfo>& edges){
        edges_ = edges;
    }

    void TransportRouter::SetVertexes(const std::unordered_map<std::string,
                     detail::Vertexes, std::hash<std::string_view>>& stop_to_vertex_id){
        stop_to_vertex_id_ = stop_to_vertex_id;
    }

	void TransportRouter::BuildRouter(){
		if (!router_ && graph_){
			router_.emplace(*graph_);
		}
	}

	void TransportRouter::Build(){
		BuildGraph();
		BuildRouter();
	}

    void TransportRouter::SetGraph(graph::DirectedWeightedGraph<double> graph){
        graph_ = graph;
    }

    std::unordered_map<std::string, detail::Vertexes, std::hash<std::string_view>>
        TransportRouter::GetStopToVertexId() const{
        return stop_to_vertex_id_;
    }

    TransportRouter::Settings TransportRouter::GetRoutingSettings() const{
        return settings_;
    }

    TransportRouter::Graph TransportRouter::GetGraph() const{
        if (graph_.has_value()){
            return graph_.value();
        }
        return {};
    }

    std::vector<detail::EdgeInfo> TransportRouter::GetEdges() const{
        return edges_;
    }
}
