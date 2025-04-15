#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include "json.hpp"


std::unordered_map<std::string, int32_t > craterForest;

// Generic node structure with templated type
template <typename T>
struct Node {
    T id;
    std::vector<Node<T>*> children;

    // Constructor
    double latitude;
    double longitude;
    double radius;

    Node(T val, double lat, double lon, double rad) : id(val), latitude(lat), longitude(lon), radius(rad) {}

    // Destructor to clean up child pointers
    ~Node() {
        for (auto child : children) {
            delete child;
        }
    }

    // Add a child node
    void addChild(Node<T>* child) {
        children.push_back(child);
    }
};

typedef std::vector<std::vector<std::string> > crater_data;

crater_data parse_csv(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening file");
    }

    std::string line;
    crater_data data;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        std::vector<std::string> row;

        while (std::getline(ss, value, ',')) {
            row.push_back(value);
        }

        data.push_back(row);
    }

    file.close();

    std::cout << "Data Length: " << std::endl;
    std::cout << data.size() << std::endl;

    return data;
}

const double MOON_RADIUS = 1737.4;

double distanceInKm(double lat1, double lon1, double lat2, double lon2) {
    // Convert degrees to radians
    lat1 *= M_PI/180.0;
    lon1 *= M_PI/180.0;
    lat2 *= M_PI/180.0;
    lon2 *= M_PI/180.0;
    
    // Haversine formula
    double dlon = lon2 - lon1;
    double dlat = lat2 - lat1;
    double a = std::pow(std::sin(dlat/2), 2) + 
               std::cos(lat1) * std::cos(lat2) * 
               std::pow(std::sin(dlon/2), 2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    
    // Distance in kilometers
    return MOON_RADIUS * c;
}

bool is_crater_a_in_crater_b(double a_lat, double a_long, double b_lat, double b_long, double b_radius) {
    double distance = distanceInKm(a_lat, a_long, b_lat, b_long);
    if (distance < b_radius) {
        return true;
    }
    return false;
}

bool nest_crater(std::string a_id, double a_lat, double a_long, double a_radius, Node<std::string>* b) {

    if (is_crater_a_in_crater_b(a_lat, a_long, b->latitude, b->longitude, b->radius)) {
        craterForest[b->id] += 1;
        if (b->children.size() == 0) {
            b->addChild(new Node<std::string>(a_id, a_lat, a_long, a_radius));
            return true;
        }   
        for (size_t i = 0; i < b->children.size(); ++i) {
            if (nest_crater(a_id, a_lat, a_long, a_radius, b->children[i])) {
                return true;
            };
        }
        b->addChild(new Node<std::string>(a_id, a_lat, a_long, a_radius));
        return true;
    }
    return false;

}


int main() {

    crater_data data = parse_csv("astro_sorted.csv");


    auto start = std::chrono::high_resolution_clock::now();
    std::vector<Node<std::string>*> crater_forest;
    std::int64_t skip_index = 0;

    // Start from 1 to skip the header
    for (size_t crater_b = 1; crater_b < 100; ++crater_b) {

        skip_index = 0;
        crater_b += skip_index;
        Node<std::string>* crater_node = new Node<std::string>(data[crater_b][1], std::stod(data[crater_b][2]), std::stod(data[crater_b][3]), std::stod(data[crater_b][6]) / 2);
        crater_forest.push_back(crater_node);
        for (size_t crater_a = crater_b + 1; crater_a < data.size(); ++crater_a) {
            if(nest_crater(data[crater_a][1], std::stod(data[crater_a][2]), std::stod(data[crater_a][3]), std::stod(data[crater_a][6]) / 2, crater_forest[crater_forest.size()-1])) {
                // Remove the crater_a that was nested from data since all craters that will be in it will get nested
                // for this crater_b
                data.erase(data.begin() + crater_a);
                
                // Important: decrement crater_a since the indexes shifted
                crater_a--;
                skip_index++;
            }
        }
    }


    nlohmann::json jsonMap(craterForest);

    std::ofstream outFile("nestedCratersLen.json");
    if (outFile.is_open()) {
        outFile << jsonMap.dump(4); // Pretty print with 4 spaces
        outFile.close();
        std::cout << "Exported successfully!" << std::endl;
    } else {
        std::cerr << "Failed to open file!" << std::endl;
    }

    return 0;
}
