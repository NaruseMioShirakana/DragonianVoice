/*
 * file: KDTree.hpp
 * author: J. Frederico Carvalho
 *
 * This is an adaptation of the KD-tree implementation in rosetta code
 * https://rosettacode.org/wiki/K-d_tree
 *
 * It is a reimplementation of the C code using C++.  It also includes a few
 * more queries than the original, namely finding all points at a distance
 * smaller than some given distance to a point.
 *
 */

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
#include <memory>
#include <vector>

#include "KDTree.hpp"

KDNode::KDNode() = default;

KDNode::KDNode(const point_t &pt, const size_t &idx_, const KDNodePtr &left_,
               const KDNodePtr &right_) {
    x = pt;
    index = idx_;
    left = left_;
    right = right_;
}

KDNode::KDNode(const pointIndex &pi, const KDNodePtr &left_,
               const KDNodePtr &right_) {
    x = pi.first;
    index = pi.second;
    left = left_;
    right = right_;
}

KDNode::~KDNode() = default;

float KDNode::coord(const size_t &idx) { return x.at(idx); }
KDNode::operator bool() { return (!x.empty()); }
KDNode::operator point_t() { return x; }
KDNode::operator size_t() { return index; }
KDNode::operator pointIndex() { return pointIndex(x, index); }

KDNodePtr NewKDNodePtr() {
    KDNodePtr mynode = std::make_shared< KDNode >();
    return mynode;
}

inline float dist2(const point_t &a, const point_t &b) {
    float distc = 0;
    for (size_t i = 0; i < a.size(); i++) {
        float di = a.at(i) - b.at(i);
        distc += di * di;
    }
    return distc;
}

inline float dist2(const KDNodePtr &a, const KDNodePtr &b) {
    return dist2(a->x, b->x);
}

inline float dist(const point_t &a, const point_t &b) {
    return std::sqrt(dist2(a, b));
}

inline float dist(const KDNodePtr &a, const KDNodePtr &b) {
    return std::sqrt(dist2(a, b));
}

comparer::comparer(size_t idx_) : idx{idx_} {};

inline bool comparer::compare_idx(const pointIndex &a,  //
                                  const pointIndex &b   //
) {
    return (a.first.at(idx) < b.first.at(idx));  //
}

inline void sort_on_idx(const pointIndexArr::iterator &begin,  //
                        const pointIndexArr::iterator &end,    //
                        size_t idx) {
    comparer comp(idx);
    comp.idx = idx;

    using std::placeholders::_1;
    using std::placeholders::_2;

    std::nth_element(begin, begin + std::distance(begin, end) / 2,
                     end, std::bind(&comparer::compare_idx, comp, _1, _2));
}

using pointVec = std::vector< point_t >;

KDNodePtr KDTree::make_tree(const pointIndexArr::iterator &begin,  //
                            const pointIndexArr::iterator &end,    //
                            const size_t &length,                  //
                            const size_t &level                    //
) {
    if (begin == end) {
        return NewKDNodePtr();  // empty tree
    }

    size_t dim = begin->first.size();

    if (length > 1) {
        sort_on_idx(begin, end, level);
    }

    auto middle = begin + (length / 2);

    auto l_begin = begin;
    auto l_end = middle;
    auto r_begin = middle + 1;
    auto r_end = end;

    size_t l_len = length / 2;
    size_t r_len = length - l_len - 1;

    KDNodePtr left;
    if (l_len > 0 && dim > 0) {
        left = make_tree(l_begin, l_end, l_len, (level + 1) % dim);
    } else {
        left = leaf;
    }
    KDNodePtr right;
    if (r_len > 0 && dim > 0) {
        right = make_tree(r_begin, r_end, r_len, (level + 1) % dim);
    } else {
        right = leaf;
    }

    // KDNode result = KDNode();
    return std::make_shared< KDNode >(*middle, left, right);
}

KDTree::KDTree(pointVec point_array) {
    leaf = std::make_shared< KDNode >();
    // iterators
    pointIndexArr arr;
    for (size_t i = 0; i < point_array.size(); i++) {
        arr.push_back(pointIndex(point_array.at(i), i));
    }

    auto begin = arr.begin();
    auto end = arr.end();

    size_t length = arr.size();
    size_t level = 0;  // starting

    root = KDTree::make_tree(begin, end, length, level);
}

KDNodePtr KDTree::nearest_(   //
    const KDNodePtr &branch,  //
    const point_t &pt,        //
    const size_t &level,      //
    const KDNodePtr &best,    //
    const float&best_dist   //
) {
    float d, dx, dx2;

    if (!bool(*branch)) {
        return NewKDNodePtr();  // basically, null
    }

    point_t branch_pt(*branch);
    size_t dim = branch_pt.size();

    d = dist2(branch_pt, pt);
    dx = branch_pt.at(level) - pt.at(level);
    dx2 = dx * dx;

    KDNodePtr best_l = best;
    float best_dist_l = best_dist;

    if (d < best_dist) {
        best_dist_l = d;
        best_l = branch;
    }

    size_t next_lv = (level + 1) % dim;
    KDNodePtr section;
    KDNodePtr other;

    // select which branch makes sense to check
    if (dx > 0) {
        section = branch->left;
        other = branch->right;
    } else {
        section = branch->right;
        other = branch->left;
    }

    // keep nearest neighbor from further down the tree
    KDNodePtr further = nearest_(section, pt, next_lv, best_l, best_dist_l);
    if (!further->x.empty()) {
        float dl = dist2(further->x, pt);
        if (dl < best_dist_l) {
            best_dist_l = dl;
            best_l = further;
        }
    }
    // only check the other branch if it makes sense to do so
    if (dx2 < best_dist_l) {
        further = nearest_(other, pt, next_lv, best_l, best_dist_l);
        if (!further->x.empty()) {
            float dl = dist2(further->x, pt);
            if (dl < best_dist_l) {
                best_dist_l = dl;
                best_l = further;
            }
        }
    }

    return best_l;
};

// default caller
KDNodePtr KDTree::nearest_(const point_t &pt) {
    size_t level = 0;
    // KDNodePtr best = branch;
    float branch_dist = dist2(point_t(*root), pt);
    return nearest_(root,          // beginning of tree
                    pt,            // point we are querying
                    level,         // start from level 0
                    root,          // best is the root
                    branch_dist);  // best_dist = branch_dist
};

point_t KDTree::nearest_point(const point_t &pt) {
    return point_t(*nearest_(pt));
};
size_t KDTree::nearest_index(const point_t &pt) {
    return size_t(*nearest_(pt));
};

pointIndex KDTree::nearest_pointIndex(const point_t &pt) {
    KDNodePtr Nearest = nearest_(pt);
    return pointIndex(point_t(*Nearest), size_t(*Nearest));
}

pointIndexArr KDTree::neighborhood_(  //
    const KDNodePtr &branch,          //
    const point_t &pt,                //
    const float&rad,                //
    const size_t &level               //
) {
    float d, dx, dx2;

    if (!bool(*branch)) {
        // branch has no point, means it is a leaf,
        // no points to add
        return pointIndexArr();
    }

    size_t dim = pt.size();

    float r2 = rad * rad;

    d = dist2(point_t(*branch), pt);
    dx = point_t(*branch).at(level) - pt.at(level);
    dx2 = dx * dx;

    pointIndexArr nbh, nbh_s, nbh_o;
    if (d <= r2) {
        nbh.push_back(pointIndex(*branch));
    }

    //
    KDNodePtr section;
    KDNodePtr other;
    if (dx > 0) {
        section = branch->left;
        other = branch->right;
    } else {
        section = branch->right;
        other = branch->left;
    }

    nbh_s = neighborhood_(section, pt, rad, (level + 1) % dim);
    nbh.insert(nbh.end(), nbh_s.begin(), nbh_s.end());
    if (dx2 < r2) {
        nbh_o = neighborhood_(other, pt, rad, (level + 1) % dim);
        nbh.insert(nbh.end(), nbh_o.begin(), nbh_o.end());
    }

    return nbh;
};

pointIndexArr KDTree::neighborhood(  //
    const point_t &pt,               //
    const float&rad) {
    size_t level = 0;
    return neighborhood_(root, pt, rad, level);
}

pointVec KDTree::neighborhood_points(  //
    const point_t &pt,                 //
    const float&rad) {
    size_t level = 0;
    pointIndexArr nbh = neighborhood_(root, pt, rad, level);
    pointVec nbhp;
    nbhp.resize(nbh.size());
    std::transform(nbh.begin(), nbh.end(), nbhp.begin(),
                   [](pointIndex x) { return x.first; });
    return nbhp;
}

indexArr KDTree::neighborhood_indices(  //
    const point_t &pt,                  //
    const float&rad) {
    size_t level = 0;
    pointIndexArr nbh = neighborhood_(root, pt, rad, level);
    indexArr nbhi;
    nbhi.resize(nbh.size());
    std::transform(nbh.begin(), nbh.end(), nbhi.begin(),
                   [](pointIndex x) { return x.second; });
    return nbhi;
}
