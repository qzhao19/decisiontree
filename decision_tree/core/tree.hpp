#ifndef CORE_TREE_HPP_
#define CORE_TREE_HPP_

#include "common/prereqs.hpp"

namespace decisiontree {

/**
 * The binary tree is represented as a number of parallel arrays. The i-th
 * element of each array holds information about the node `i`. Node 0 is the
 * tree's root. 
 * 
 * For node data stored at index i, the two child nodes are at 
 * index (2 * i + 1) and (2 * i + 2); the parent node is (i - 1) // 2 
 * (where // indicates integer division).
*/
class Tree {
private:
    struct IndexInfo {
        NodeIndexType index;
        double weight;
        IndexInfo(NodeIndexType index, 
                  double weight) : 
            index(index), 
            weight(weight) {};
        ~IndexInfo() {};
    };

    struct TreeNode {
        NodeIndexType left_child;
        NodeIndexType right_child;
        FeatureIndexType feature_index;
        int has_missing_value;
        FeatureType threshold;
        double impurity;
        double improvement;
        std::vector<std::vector<HistogramType>> histogram;

        TreeNode(NodeIndexType left_child, 
                 NodeIndexType right_child, 
                 FeatureIndexType feature_index,
                 int has_missing_value,
                 FeatureType threshold,
                 double impurity,
                 double improvement,
                 const std::vector<std::vector<HistogramType>>& histogram): 
            left_child(left_child), 
            right_child(right_child), 
            feature_index(feature_index),
            has_missing_value(has_missing_value),
            threshold(threshold),
            impurity(impurity),
            improvement(improvement),
            histogram(histogram) {};
        
        ~TreeNode() {};
    };

    NumOutputsType num_outputs_;
    NumFeaturesType num_features_;
    std::vector<NumClassesType> num_classes_list_;

    TreeDepthType max_depth_;
    NodeIndexType node_count_;
    NumClassesType max_num_classes_;

    friend std::ostream& operator<<(std::ostream& os, const TreeNode& node){
        return os << "left child = " << node.left_child 
                  << ", right child = " << node.right_child 
                  << ", feature index = " << node.feature_index
                  << ", threshold = " << node.threshold
                  << ", improvement = " << node.improvement
                  << ", histogram size = (" << node.histogram.size() 
                  << ", " << node.histogram[0].size() << ")";
    }

public:
    std::vector<TreeNode> nodes_;

    Tree() {};
    Tree(NumOutputsType num_outputs, 
         NumFeaturesType num_features, 
         std::vector<NumClassesType> num_classes_list): 
            num_outputs_(num_outputs),
            num_features_(num_features),
            num_classes_list_(num_classes_list) {
        max_depth_ = 0;
        node_count_ = 0;
        max_num_classes_ = *std::max_element(std::begin(num_classes_list), std::end(num_classes_list));
    };
    ~Tree() {
        nodes_.clear();
    };

    NodeIndexType add_node(bool is_left,
                         TreeDepthType depth, 
                         NodeIndexType parent_index, 
                         FeatureIndexType feature_index, 
                         int has_missing_value, 
                         FeatureType threshold, 
                         double impurity, 
                         double improvement, 
                         const std::vector<std::vector<HistogramType>>& histogram) {

        nodes_.emplace_back(0, 0, 
                            feature_index, 
                            has_missing_value, 
                            threshold, impurity, 
                            improvement, 
                            histogram);
        NodeIndexType node_index = node_count_++;
        
        // not root node
        if (depth > 0) {
            if (is_left) {
                nodes_[parent_index].left_child = node_index;
            }
            else {
                nodes_[parent_index].right_child = node_index;
            }
        }

        if (depth > max_depth_) {
            max_depth_ = depth;
        }

        return node_index;
    }

    /**
     * the importances of features is computed as the total improvement 
     * of criterion brought by the feature.
    */
    void compute_feature_importance(std::vector<double>& importances) {
        importances.resize(num_features_, 0.0);
        if (node_count_ == 0) {
            return;
        }
        
        // loop all node
        for (IndexType i = 0; i < node_count_; ++i) {
            // loop all non-leaf node, accumulate improvement per features
            if (nodes_[i].left_child > 0) {
                importances[nodes_[i].feature_index] += nodes_[i].improvement;
            }
        }

        // normalizer
        double norm_coeff = 0.0;
        for (IndexType i = 0; i < num_features_; ++i) {
            norm_coeff += importances[i];
        }
        if (norm_coeff > 0.0) {
            for (IndexType i = 0; i < num_features_; ++i) {
                importances[i] = importances[i] / norm_coeff;
            }
        }
    };


    void predict_proba(const std::vector<FeatureType>& X, 
                       NumSamplesType num_samples,
                       std::vector<double>& proba) {
        proba.resize(num_samples * num_outputs_ * max_num_classes_, 0.0);
        
        // loop samples
        for (IndexType i = 0; i < num_samples; ++i) {
            std::stack<IndexInfo> node_index_stk;
            std::stack<IndexInfo> leaf_index_stk;

            // goto root node
            node_index_stk.emplace(IndexInfo(0, 1.0));

            // loop root to leaf node
            while (!node_index_stk.empty()) {
                IndexInfo node_index = node_index_stk.top();
                node_index_stk.pop();

                while (nodes_[node_index.index].left_child > 0 && nodes_[node_index.index].right_child > 0) {
                    if (std::isnan(X[i * num_features_ + nodes_[node_index.index].feature_index])) {
                        // split criterion which includes missing values
                        // suppose has_missing_value = 0, missing value is at left node
                        // has_missing_value = 1, missing value is at right node
                        if (nodes_[node_index.index].has_missing_value == 0) {
                            node_index.index = nodes_[node_index.index].left_child;
                        }
                        else if (nodes_[node_index.index].has_missing_value == 1) {
                            node_index.index = nodes_[node_index.index].right_child;
                        }
                        else {
                            // split criterion which does not include missing values
                            IndexInfo node_index2 = node_index;
                            node_index2.index = nodes_[node_index.index].right_child;
                            HistogramType num_rights = std::accumulate(nodes_[node_index2.index].histogram[0].begin(),
                                                                       nodes_[node_index2.index].histogram[0].end(),
                                                                       0.0);

                            node_index.index = nodes_[node_index.index].left_child;
                            HistogramType num_lefts = std::accumulate(nodes_[node_index.index].histogram[0].begin(),
                                                                       nodes_[node_index.index].histogram[0].end(),
                                                                       0.0);
                            
                            node_index.weight *= num_lefts / (num_lefts + num_rights);
                            node_index2.weight *= num_rights / (num_lefts + num_rights);
                            node_index_stk.emplace(node_index2);
                        }
                    } 
                    else {
                        // no missing samples
                        // if x[i, f_index] is smaller than the partition threshold of current node
                        // goto left child node, else goto right child node
                        if (X[i * num_features_ + nodes_[node_index.index].feature_index] <= nodes_[node_index.index].threshold) {
                            node_index.index = nodes_[node_index.index].left_child;
                        }
                        else {
                            node_index.index = nodes_[node_index.index].right_child;
                        }
                    }
                }
                // store leaf nodes
                leaf_index_stk.emplace(node_index);
            }
            while (!leaf_index_stk.empty()) {
                IndexInfo leaf_index = leaf_index_stk.top();
                leaf_index_stk.pop();

                for (IndexType o = 0; o < num_outputs_; ++o) {
                    double norm_coeff = 0.0;
                    for (NumClassesType c = 0; c < num_classes_list_[o]; ++c) {
                        norm_coeff += nodes_[leaf_index.index].histogram[o][c];
                        // std::cout << "norm_coeff = " << norm_coeff << std::endl;
                    }
                    if (norm_coeff > 0.0) {
                        for (NumClassesType c = 0; c < num_classes_list_[o]; ++c) {
                            proba[i * num_outputs_ * max_num_classes_ + o * max_num_classes_ + c] += leaf_index.weight * 
                                nodes_[leaf_index.index].histogram[o][c] / norm_coeff;
                        }
                    }
                }
            }
        }
    };

    void print_node_info() {
        for (IndexType i = 0; i < nodes_.size(); i++) {
            std::cout << nodes_[i] << std::endl;
        }
    };


};

} // namespace decision-tree

#endif // CORE_TREE_HPP_