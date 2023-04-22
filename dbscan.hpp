#ifndef DBSCAN_HPP
#define DBSCAN_HPP

#include "kdtree.hpp"
#include "nanoflann/nanoflann.hpp" // nanoflann::KDTreeEigenMatrixAdaptor
#include "point_struct.hpp"        // PointCloud
#include <cstdint>                 // std::int32_t, std::size_t
#include <iostream>                // std::cout
#include <unordered_map>           // std::unordered_map
#include <utility>                 // std::pair
#include <vector>                  // std::vector

namespace clustering
{
namespace labels
{
constexpr std::int32_t UNDEFINED = -2;
constexpr std::int32_t NOISE = -1;
} // namespace labels

template <typename CoordinateType, std::size_t NUMBER_OF_DIMENSIONS> class DBSCAN final
{
  public:
    constexpr static std::int32_t MAX_LEAF_SIZE = 10;
    constexpr static std::int32_t IGNORE_CHECKS = 32;
    constexpr static float USE_APPROXIMATE_SEARCH = 0.0f;
    constexpr static bool SORT_RESULTS = true;

    DBSCAN(const DBSCAN &) = delete;
    DBSCAN &operator=(const DBSCAN &) = delete;
    DBSCAN(DBSCAN &&) = delete;
    DBSCAN &operator=(DBSCAN &&) = delete;
    DBSCAN() = delete;

    explicit DBSCAN(const CoordinateType distance_threshold, const std::int32_t min_neighbour_points,
                    const PointCloud<CoordinateType, NUMBER_OF_DIMENSIONS> &points)
        : distance_threshold_squared_(distance_threshold * distance_threshold),
          min_neighbour_points_(min_neighbour_points), points_(points),
          //   kdtree_index_(NUMBER_OF_DIMENSIONS /*dim*/, points_, {MAX_LEAF_SIZE /*max leaf*/}),
          //   search_parameters_{IGNORE_CHECKS /*disable checks*/, USE_APPROXIMATE_SEARCH /*use approximated search?*/,
          //  SORT_RESULTS /*sort*/},
          kdtree_{points.points, false}
    {
    }

    ~DBSCAN() = default;

    const auto getClusterIndices() const
    {
        return cluster_indices_;
    }

    void formClusters()
    {
        // Must not have less that 2 points
        if (points_.points.size() < 2)
        {
            return;
        }

        // Set all initial labels to UNDEFINED
        std::vector<std::int32_t> labels(points_.points.size(), labels::UNDEFINED);
        auto labels_it = labels.begin();

        // Reserve memory for neighbors
        // std::vector<std::pair<std::uint32_t, CoordinateType>> neighbors;
        // neighbors.reserve(1000);

        std::vector<std::pair<std::size_t, CoordinateType>> neighbors_;
        neighbors_.reserve(1000);

        // std::vector<std::pair<std::uint32_t, CoordinateType>> inner_neighbors;
        // inner_neighbors.reserve(1000);

        std::vector<std::pair<std::size_t, CoordinateType>> inner_neighbors_;
        inner_neighbors_.reserve(1000);

        // Initial cluster counter
        std::int32_t label = 0;

        // Iterate over each point
        for (std::int32_t index = 0; index < points_.points.size(); ++index)
        {
            // Check if label is not undefined
            auto current_labels_it = labels_it + index;
            if (*current_labels_it != labels::UNDEFINED)
            {
                continue;
            }

            // Find nearest neighbors within radius
            // neighbors.clear();
            neighbors_.clear();

            // Check density
            kdtree_.findAllNearestNeighboursWithinRadiusSquared(points_.points[index], distance_threshold_squared_,
                                                                neighbors_);

            // if (kdtree_index_.radiusSearch(points_.points[index].data(), distance_threshold_squared_, neighbors,
            //                                search_parameters_) < min_neighbour_points_)
            if (neighbors_.size() < min_neighbour_points_)
            {
                // Label query point as noise
                *current_labels_it = labels::NOISE;
                continue;
            }

            // Set the next cluster label
            ++label;

            // Label initial point
            *current_labels_it = label;

            // Exclude the first point from the radius search, and iterate over all neighbors
            for (auto neighbor_it = neighbors_.cbegin(); neighbor_it != neighbors_.cend(); ++neighbor_it)
            {
                const auto &neighbor_index = (*neighbor_it).first;
                auto current_neighbor_labels_it = labels_it + neighbor_index;

                if (*current_neighbor_labels_it == labels::NOISE)
                {
                    // Change noise to border point
                    *current_neighbor_labels_it = label;
                    continue;
                }

                // Previously processed, border point
                if (*current_neighbor_labels_it != labels::UNDEFINED)
                {
                    continue;
                }

                // Label neighbor
                *current_neighbor_labels_it = label;

                // Find neighbors
                // inner_neighbors.clear();
                inner_neighbors_.clear();

                // Density check, if inner_query_point is a core point
                kdtree_.findAllNearestNeighboursWithinRadiusSquared(points_.points[neighbor_index],
                                                                    distance_threshold_squared_, inner_neighbors_);

                // if (kdtree_index_.radiusSearch(points_.points[neighbor_index].data(), distance_threshold_squared_,
                //                                inner_neighbors, search_parameters_) >= min_neighbour_points_)

                if (inner_neighbors_.size() >= min_neighbour_points_)
                {
                    // Add new neighbors to the seed set
                    for (auto inner_neighbor_it = inner_neighbors_.cbegin() + 1;
                         inner_neighbor_it != inner_neighbors_.cend(); ++inner_neighbor_it)
                    {
                        *(labels_it + (*inner_neighbor_it).first) = label;
                    }
                }
            }
        }

        // Iterate over indices and copy to cluster indices
        for (std::int32_t i = 0; i < labels.size(); ++i)
        {
            cluster_indices_[labels[i]].push_back(i);
        }

        std::cout << "Number of clusters: " << cluster_indices_.size() << std::endl;
    }

  private:
    const CoordinateType distance_threshold_squared_;
    const std::int32_t min_neighbour_points_;
    const PointCloud<CoordinateType, NUMBER_OF_DIMENSIONS> points_;
    // const nanoflann::KDTreeSingleIndexAdaptor<
    //     nanoflann::L2_Adaptor<CoordinateType, PointCloud<CoordinateType, NUMBER_OF_DIMENSIONS>>,
    //     PointCloud<CoordinateType, NUMBER_OF_DIMENSIONS>, NUMBER_OF_DIMENSIONS /* dim */>
    //     kdtree_index_;
    // const nanoflann::SearchParams search_parameters_;

    neighbour_search::KDTree<CoordinateType, NUMBER_OF_DIMENSIONS> kdtree_;
    std::unordered_map<std::int32_t, std::vector<std::int32_t>> cluster_indices_;
};
} // namespace clustering

#endif // DBSCAN_HPP