#include "transport_catalogue.h"

#include <algorithm>
#include <utility>
#include <iostream>

namespace transport_catalogue{

	namespace detail{
		size_t StringPairHash::operator() (const std::pair<std::string, std::string>& stops) const {
			return hash_s(stops.first) + hash_s(stops.second) * 101;
		}
		size_t StopHash::operator() (const std::pair<Stop*, Stop*>& stops) const {
			return hash_v(stops.first) + hash_v(stops.second) * 101;
		}
	}

	void TransportCatalogue::AddRoute(const Bus& bus){
		buses.push_back(bus);

		// calculate length
		double length_c = 0;
		double length_f = 0;

		// one stop on route handler
		if (buses.back().stops.size() == 1){
			const auto stop = FindStop(buses.back().stops[0]->name);
			auto found_stop = stop_to_buses.find(stop->name);
			if (found_stop != stop_to_buses.end()){
				found_stop->second.insert(&buses.back());
			}
		}

		// if route stops is equal to 1 -> doesnt reach here
		for(size_t i = 1; i < buses.back().stops.size(); i++){
			const auto prev_stop = FindStop(buses.back().stops[i - 1]->name);
			const auto stop = FindStop(buses.back().stops[i]->name);

			length_c += geo::ComputeDistance(prev_stop->coordinates, stop->coordinates);

			const auto prev_stop_to_stop_distance = GetDistance(std::make_pair(prev_stop, stop));
			const auto stop_to_prev_stop_distance = GetDistance(std::make_pair(stop, prev_stop));

			if (!prev_stop_to_stop_distance.has_value()
					&& !stop_to_prev_stop_distance.has_value()){
				length_f += -1; // set by authors
			}else{
				if (prev_stop_to_stop_distance.has_value()){
					length_f += prev_stop_to_stop_distance.value();
				}else{
					length_f += stop_to_prev_stop_distance.value();
				}
			}

			auto found_stop = stop_to_buses.find(stop->name);
			if (found_stop != stop_to_buses.end()){
				found_stop->second.insert(&buses.back());
			}
		}

		buses.back().factual_length	 = length_f;
		buses.back().length_by_coordinates = length_c;
		routes[buses.back().name] = &buses.back();
	}

	void TransportCatalogue::AddStop(const Stop& stop){
		stops.push_back(stop);
		stops_by_names[stops.back().name] = &stops.back();
		if (stop_to_buses.find(stops.back().name) == stop_to_buses.end()){
			stop_to_buses[stops.back().name] = {};
		}
	}

	Stop* TransportCatalogue::FindStop(const std::string_view stop_name) const {
		if (stops_by_names.count(stop_name) != 0){
			return stops_by_names.at(stop_name);
		}
		return nullptr;
	}

	Bus* TransportCatalogue::FindRoute(const std::string_view bus_name) const {
		if (routes.count(bus_name) != 0){
			return routes.at(bus_name);
		}
		return nullptr;
	}

	std::pair<std::string_view, const std::optional<Bus*>>
	TransportCatalogue::GetRouteInfo(const std::string_view bus_name) const {
		if (routes.count(bus_name)){
			return {bus_name, FindRoute(bus_name)};
		}
		return {bus_name, std::nullopt};
	}

	std::pair<std::string_view, const std::optional<std::set<std::string_view>>>
	TransportCatalogue::GetStopInfo(const std::string_view stop_name) const {
		if (stop_to_buses.count(stop_name)){
			std::set<std::string_view> buses;
			for (const auto& bus : stop_to_buses.at(stop_name)){
				buses.insert(bus->name);
			}

			return {stop_name, buses};
		}
		return {stop_name, std::nullopt};
	}

	void TransportCatalogue::SetDistances(const std::unordered_map<std::pair<std::string, std::string>, int,
			detail::StringPairHash>& pair_from_to) {
		for (const auto& [key, val] : pair_from_to) {
			distances[std::make_pair(FindStop(key.first), FindStop(key.second))] = val;
		}
	}

	std::optional<int> TransportCatalogue::GetDistance(const std::pair<Stop*, Stop*>& pair_from_to) const {
		const auto elem = distances.find(pair_from_to);
		if (elem != distances.end()){
			return (*elem).second;
		}
		return {};
	}

    std::unordered_map<std::pair<Stop*, Stop*>, int, detail::StopHash> TransportCatalogue::GetDistances() const{
        return distances;
    }

    std::unordered_set<Bus*> TransportCatalogue::GetBusesOnStop(const std::string& stop_name) const {
		return stop_to_buses.at(stop_name);
	}

	std::map<std::string, Bus*> TransportCatalogue::GetSortedBuses() const{
		std::map<std::string, Bus*> buses_sorted;
		for (const auto& [bus_name_view, bus] : routes) {
			buses_sorted.emplace(std::make_pair(
					std::string(bus_name_view.begin(), bus_name_view.end()), bus));
		}
		return buses_sorted;
	}

	std::deque<Stop> TransportCatalogue::GetStops() const{
		return stops;
	}

	std::deque<Bus> TransportCatalogue::GetBuses() const{
		return buses;
	}

	std::unordered_map<std::string_view, Stop*> TransportCatalogue::GetStopsByNames() const{
		return stops_by_names;
	}

	std::unordered_map<std::string_view, Bus*> TransportCatalogue::GetRoutes() const{
		return routes;
	}
}

