#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport_catalogue{

	struct Stop{
		std::string name;
		geo::Coordinates coordinates;
	};

	class Bus{
	public:
		std::string name;
		std::vector<Stop*> stops;
		size_t unique_stops = 0;
		size_t stops_on_route = 0;
		double length_by_coordinates;
		double factual_length;
		bool is_round = false;
	};

	inline bool operator ==(const Stop& lhs, const Stop& rhs){
		return lhs.name == rhs.name;
	}

	inline bool operator==(const Bus& lhs, const Bus& rhs){
		return lhs.name == rhs.name;
	}
}
