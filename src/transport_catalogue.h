#pragma once
#include "geo.h"
#include "domain.h"

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace transport_catalogue{

	namespace detail{
		struct StringPairHash {
			size_t operator () (const std::pair<std::string, std::string>&) const;
			std::hash<std::string> hash_s;
		};

		struct StopHash {
			size_t operator() (const std::pair<Stop*, Stop*>&) const;
			std::hash<const void*> hash_v;
		};
	}

	class TransportCatalogue{
	public:
		void AddStop(const Stop& stop);
		void AddRoute(const Bus& bus);
		Stop* FindStop(const std::string_view stop_name) const;
		Bus* FindRoute(const std::string_view bus_name) const;
		std::pair<std::string_view, const std::optional<Bus*>> GetRouteInfo(const std::string_view bus_name) const;
		std::pair<std::string_view, const std::optional<std::set<std::string_view>>> GetStopInfo(const std::string_view stop_name) const;
		void SetDistances(const std::unordered_map<std::pair<std::string, std::string>, int, detail::StringPairHash>& input);
		std::optional<int> GetDistance(const std::pair<Stop*, Stop*>& pair_from_to) const;
        std::unordered_map<std::pair<Stop*, Stop*>, int, detail::StopHash> GetDistances() const;
		std::unordered_set<Bus*> GetBusesOnStop(const std::string& stop_name) const;
		std::map<std::string, Bus*> GetSortedBuses() const;
		std::deque<Stop> GetStops() const;
		std::deque<Bus> GetBuses() const;
		std::unordered_map<std::string_view, Stop*> GetStopsByNames() const;
		std::unordered_map<std::string_view, Bus*> GetRoutes() const;

	private:
		std::deque<Stop> stops;
		std::deque<Bus> buses;
		std::unordered_map<std::string_view, Bus*> routes;
		std::unordered_map<std::string_view, Stop*> stops_by_names;
		std::unordered_map<std::string_view, std::unordered_set<Bus*>> stop_to_buses;
		std::unordered_map<std::pair<Stop*, Stop*>, int, detail::StopHash> distances;
	};
}
