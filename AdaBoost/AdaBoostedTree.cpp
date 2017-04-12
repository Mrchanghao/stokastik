// [[Rcpp::plugins(cpp11)]]
#include <Rcpp.h>
#include <fstream>
#include <queue>
#include <stack>

using namespace Rcpp;

typedef std::unordered_map<int, std::unordered_map<double, std::set<int>>> Int_Double_SetInt;
typedef std::unordered_map<int, std::set<double>> Int_SetDouble;
typedef std::unordered_map<int, std::set<int>> Int_SetInt;
typedef std::unordered_map<int, std::unordered_map<int, double>> Int_Int_Double;
typedef std::unordered_map<int, double> Int_Double;
typedef std::unordered_map<int, int> Int_Int;
typedef std::unordered_map<int, std::unordered_map<double, double>> Int_Double_Double;
typedef std::unordered_map<double, std::set<int>> Double_SetInt;
typedef std::unordered_map<double, double> Double_Double;

struct Split {
  int featureIndex;
  double featureDecisionVal;
  
  std::set<int> leftRows, rightRows;
};

struct Node {
  
  Split bestSplit;
  
  Int_Double classProbs;
  
  int depth, numLeavesBranch;
  double resubErrorBranch;
  
  std::set<int> rows;
  
  Node* left;
  Node* right;
};

struct InputMetaData {
  Int_Double_SetInt colValAllRowsMap;
  Int_SetDouble colSortedVals;
  Int_SetInt rowCols;
  Int_Int_Double rowColValMap;
  Int_Double instanceWeights;
  Int_Int rowClassMap;
  
  std::set<int> classLabels;
};

Node* copyNode(Node* a) {
  Node* b = new Node();
  
  if (a == NULL) b = NULL;
  else {
    b->bestSplit = a->bestSplit;
    b->classProbs = a->classProbs;
    b->depth = a->depth;
    b->numLeavesBranch = a->numLeavesBranch;
    b->resubErrorBranch =  a->resubErrorBranch;
    b->rows =  a->rows;
    
    b->left = copyNode(a->left);
    b->right = copyNode(a->right);
  }
  
  return b;
}

Int_Double computeClassProbs(const std::set<int> &rows, InputMetaData &metaData) {
  
  Int_Double classProbs;
  
  for (auto p = metaData.classLabels.begin(); p != metaData.classLabels.end(); ++p) classProbs[*p] = 0;
  
  double weightSum = std::accumulate(rows.begin(), rows.end(), 0.0, 
                                     [&metaData](double weightSum, const int &row){return weightSum + metaData.instanceWeights[row];});
  
  for (auto p = rows.begin(); p != rows.end(); ++p) classProbs[metaData.rowClassMap[*p]] += metaData.instanceWeights[*p]/weightSum;
  
  return classProbs;
}

double giniImpurity(const std::set<int> &rows, InputMetaData &metaData) {
  
  Int_Double classProbs = computeClassProbs(rows, metaData);
  
  double impurity = std::accumulate(classProbs.begin(), classProbs.end(), 0.0, 
                                    [](double impurity, const std::pair<int, double> &a){return impurity + a.second*(1-a.second);});
  
  return impurity;
}

double entropy(const std::set<int> &rows, InputMetaData &metaData) {
  
  Int_Double classProbs = computeClassProbs(rows, metaData);
  
  double impurity = std::accumulate(classProbs.begin(), classProbs.end(), 0.0, 
                                    [](double impurity, const std::pair<int, double> &a){return impurity - a.second*log2(a.second);});
  
  return impurity;
}

std::pair<int, double> bestClass(Int_Double &classProbs) {
  
  auto it = std::max_element(classProbs.begin(), classProbs.end(), 
                             [](const std::pair<int, double> &a, const std::pair<int, double> &b){return a.second < b.second;});
  
  return *it;
}

Node* getLeafNode(const std::set<int> &rows, InputMetaData &metaData) {
  
  Node *node = new Node();
  
  node->bestSplit = {-1, -1, std::set<int>(), std::set<int>()};
  
  node->classProbs = computeClassProbs(rows, metaData);
  
  node->rows = rows;
  
  node->left = NULL;
  node->right = NULL;
  
  return node;
}

Node* createNode(const std::set<int> &currRows,
                 InputMetaData &metaData,
                 double (*costFuntion) (const std::set<int> &, InputMetaData &),
                 const bool &maxDepthReached) {
  
  Node *node = new Node();
  
  Int_Double classProbs = computeClassProbs(currRows, metaData);
  
  if ((int)classProbs.size() == 1 || maxDepthReached) node = getLeafNode(currRows, metaData);
  
  else {
    double maxFeatureGain = 0;
    double featureDecisionVal = -1;
    int featureIdx = -1;
    
    std::set<int> leftRows, rightRows, thisCols;
    
    Int_SetDouble thisColValsMap;
    
    for (auto p = currRows.begin(); p != currRows.end(); ++p) {
      std::set<int> cols = metaData.rowCols[*p];
      thisCols.insert(cols.begin(), cols.end());
      
      for (auto q = cols.begin(); q != cols.end(); ++q) thisColValsMap[*q].insert(metaData.rowColValMap[*p][*q]);
    }
    
    std::vector<int> colsVec(thisCols.begin(), thisCols.end());
    
    std::random_shuffle(colsVec.begin(), colsVec.end());
    
    std::set<int> thisColsSampled(colsVec.begin(), colsVec.begin()+0.1*colsVec.size());
    
    for (auto p = thisColsSampled.begin(); p != thisColsSampled.end(); ++p) {
      
      int feature = *p;
      
      std::set<double> sortedVals = thisColValsMap[feature];
      
      std::set<int> allRows = metaData.colValAllRowsMap[feature][*sortedVals.rbegin()];
      
      std::set<int> thisRows, sparseRows;
      
      std::set_intersection(currRows.begin(), currRows.end(), allRows.begin(), allRows.end(),
                            std::inserter(thisRows, thisRows.end()));
      
      std::set_difference(currRows.begin(), currRows.end(), thisRows.begin(), thisRows.end(),
                          std::inserter(sparseRows, sparseRows.end()));
      
      if (sparseRows.size() > 0) sortedVals.insert(0);
      
      double maxGain = 0;
      double decisionVal = -1;
      
      std::set<int> bestLeftCostRows, bestRightCostRows;
      
      for (auto q = sortedVals.begin(); q != sortedVals.end(); ++q) {
        
        std::set<int> leftCostRows, rightCostRows;
        
        if (*q > 0) {
          std::set<int> leftR = metaData.colValAllRowsMap[feature][*q];
          
          std::set_intersection(currRows.begin(), currRows.end(), leftR.begin(), leftR.end(),
                                std::inserter(leftCostRows, leftCostRows.end()));
        }
        
        leftCostRows.insert(sparseRows.begin(), sparseRows.end());
        
        double leftCost = costFuntion(leftCostRows, metaData);
        
        std::set_difference(currRows.begin(), currRows.end(), leftCostRows.begin(), leftCostRows.end(), 
                            std::inserter(rightCostRows, rightCostRows.end()));
        
        double rightCost = costFuntion(rightCostRows, metaData);
        
        double w1 = (double)leftCostRows.size()/(double)currRows.size();
        double w2 = (double)rightCostRows.size()/(double)currRows.size();
        
        double totalImpurity=costFuntion(currRows, metaData);
        
        double gain = totalImpurity-(w1*leftCost+w2*rightCost);
        
        if (gain > maxGain) {
          maxGain = gain;
          decisionVal = *q;
          bestLeftCostRows = leftCostRows;
          bestRightCostRows = rightCostRows;
        }
      }
      
      if (maxGain > maxFeatureGain && decisionVal < *sortedVals.rbegin()) {
        maxFeatureGain = maxGain;
        featureDecisionVal = decisionVal;
        featureIdx = feature;
        leftRows = bestLeftCostRows;
        rightRows = bestRightCostRows;
      }
    }
    
    if (featureIdx != -1) {
      
      std::set<double> sortedVals = thisColValsMap[featureIdx];
      
      node->bestSplit = {featureIdx, featureDecisionVal, leftRows, rightRows};
      
      node->bestSplit.featureIndex = featureIdx;
      node->bestSplit.featureDecisionVal = featureDecisionVal;
      
      node->rows = currRows;
      
      node->classProbs = computeClassProbs(currRows, metaData);
    }
    
    else node = getLeafNode(currRows, metaData);
  }
  
  return node;
}

Node* constructTree(const std::set<int> &rows,
                    InputMetaData &metaData, 
                    double (*costFuntion) (const std::set<int> &, InputMetaData &),
                    const int &maxDepth) {
  
  std::queue<Node*> nodeQ;
  
  Node *node = createNode(rows, metaData, costFuntion, false);
  node->depth = 0;
  
  Node* root = node;
  
  nodeQ.push(node);
  
  while(!nodeQ.empty()) {
    node = nodeQ.front();
    nodeQ.pop();
    
    if (node->bestSplit.featureDecisionVal != -1) {
      bool maxDepthReached = (node->depth == maxDepth-1);
      
      node->left = createNode(node->bestSplit.leftRows, metaData, costFuntion, maxDepthReached);
      node->right = createNode(node->bestSplit.rightRows, metaData, costFuntion, maxDepthReached);
      
      node->left->depth = node->depth + 1;
      node->right->depth = node->depth + 1;
      
      nodeQ.push(node->left);
      nodeQ.push(node->right);
    }
  }
  
  return root;
}

DataFrame transformTreeIntoDF(Node* node, DataFrame &leafNodeClassProbs) {
  
  std::vector<int> nodeIndex, leftNodeIndex, rightNodeIndex, featureIndex, leafLabels, leafIndices;
  std::vector<double> featureDecisionVal, leafLabelProbs;
  
  std::queue<Node*> nodeQ;
  nodeQ.push(node);
  
  int index = 0;
  int lastIndex = 0;
  
  while (!nodeQ.empty()) {
    Node* n = nodeQ.front();
    
    nodeIndex.push_back(index);
    
    featureIndex.push_back(n->bestSplit.featureIndex);
    featureDecisionVal.push_back(n->bestSplit.featureDecisionVal);
    
    if (n->left != NULL) {
      leftNodeIndex.push_back(++lastIndex);
      nodeQ.push(n->left);
      
      rightNodeIndex.push_back(++lastIndex);
      nodeQ.push(n->right);
    }
    
    else {
      leftNodeIndex.push_back(-1);
      rightNodeIndex.push_back(-1);
      
      std::unordered_map<int, double> predProbs = n->classProbs;
      
      for (auto p = predProbs.begin(); p != predProbs.end(); ++p) {
        leafLabels.push_back(p->first);
        leafIndices.push_back(index);
        leafLabelProbs.push_back(p->second);
      }
    }
    
    nodeQ.pop();
    index++;
  }
  
  leafNodeClassProbs = DataFrame::create(_["LeafIndex"]=leafIndices, _["LeafLabel"]=leafLabels, _["LeafLabelProb"]=leafLabelProbs);
  
  return DataFrame::create(_["NodeIndex"]=nodeIndex, _["LeftNodeIndex"]=leftNodeIndex, 
                           _["RightNodeIndex"]=rightNodeIndex, 
                           _["FeatureIndex"]=featureIndex, 
                           _["FeatureDecisionVal"]=featureDecisionVal);
}

Int_Double treePredict(Node* node, Int_Double &colValMap) {
  
  if (node->bestSplit.featureIndex == -1) return node->classProbs;
  
  else {
    int decisionFeature = node->bestSplit.featureIndex;
    double decisionVal = node->bestSplit.featureDecisionVal;
    
    if (colValMap[decisionFeature] <= decisionVal) return treePredict(node->left, colValMap);
    else return treePredict(node->right, colValMap);
  }
}

double resubstitutionErrorNode(Node* node, 
                               InputMetaData &metaData, int totalRows) {
  
  Int_Double classProbs = computeClassProbs(node->rows, metaData);
  std::pair<int, double> best = bestClass(classProbs);
  
  return (1-best.second)*(double)node->rows.size()/(double)totalRows;
}

Node* resubstitutionErrorBranches(Node* node, InputMetaData &metaData, int totalRows) {
  
  if (node->left == NULL) node->resubErrorBranch = resubstitutionErrorNode(node, metaData, totalRows);
  
  else {
    Node* n1 = resubstitutionErrorBranches(node->left, metaData, totalRows);
    Node* n2 = resubstitutionErrorBranches(node->right, metaData, totalRows);
    
    node->resubErrorBranch = n1->resubErrorBranch + n2->resubErrorBranch;
  }
  
  return node;
}

Node* nodeNumLeaves(Node* node) {
  
  if (node->left == NULL) node->numLeavesBranch = 1;
  else node->numLeavesBranch = nodeNumLeaves(node->left)->numLeavesBranch + nodeNumLeaves(node->right)->numLeavesBranch;
  
  return node;
}

std::pair<Node*, double> getMinComplexityNodes(Node* node, InputMetaData &metaData) {
  
  Node* n = node;
  std::queue<Node*> nodeQ;
  
  nodeQ.push(n);
  
  int totalRows = node->rows.size();
  
  double minComplexity = std::numeric_limits<double>::max();
  Node* minComplexityNode = new Node();
  
  while(!nodeQ.empty()) {
    Node* n = nodeQ.front();
    nodeQ.pop();
    
    double a = resubstitutionErrorNode(n, metaData, totalRows);
    double b = n->resubErrorBranch;
    int c = n->numLeavesBranch;
    
    double g = (a-b)/((double)c-1);
    
    if (g < minComplexity || (g == minComplexity && n->depth < minComplexityNode->depth)) {
      minComplexity = g;
      minComplexityNode = n;
    }
    
    if (n->left != NULL && n->left->left != NULL) nodeQ.push(n->left);
    if (n->right != NULL && n->right->left != NULL) nodeQ.push(n->right);
  }
  
  return std::make_pair(minComplexityNode, minComplexity);
}

Node* pruneNode(Node* root, Node* node, InputMetaData &metaData) {
  if (node == NULL || root->left == NULL) return root;
  else if (root->bestSplit.featureIndex == node->bestSplit.featureIndex 
             && root->bestSplit.featureDecisionVal == node->bestSplit.featureDecisionVal 
             && root->rows.size() == node->rows.size() 
             && root->depth == node->depth) return getLeafNode(root->rows, metaData);
  else {
    root->left = pruneNode(root->left, node, metaData);
    root->right = pruneNode(root->right, node, metaData);
    
    return root;
  }
}

std::vector<std::pair<Node*, double>> complexityPruneTreeSeq(Node* root, InputMetaData &metaData) {
  std::vector<std::pair<Node*, double>> output;
  
  root = resubstitutionErrorBranches(root, metaData, root->rows.size());
  root = nodeNumLeaves(root);
  
  output.push_back(std::make_pair((Node*)NULL, 0));
  
  while(root->left != NULL) {
    std::pair<Node*, double> minComplexityNodes = getMinComplexityNodes(root, metaData);
    
    root = pruneNode(root, minComplexityNodes.first, metaData);
    
    root = resubstitutionErrorBranches(root, metaData, root->rows.size());
    root = nodeNumLeaves(root);
    
    output.push_back(std::make_pair(minComplexityNodes.first, minComplexityNodes.second));
  }
  
  return output;
}

std::vector<std::pair<Node*, int>> complexityPruneTreeSeqWithComplexities(Node* root, InputMetaData &metaData, 
                                                                          const std::vector<double> &complexities) {
  
  std::vector<std::pair<Node*, int>> output;
  
  root = resubstitutionErrorBranches(root, metaData, root->rows.size());
  root = nodeNumLeaves(root);
  
  output.push_back(std::make_pair((Node*)NULL, 0));
  
  int i = 0;
  
  while(root->left != NULL) {
    std::pair<Node*, double> minComplexityNodes = getMinComplexityNodes(root, metaData);
    
    root = pruneNode(root, minComplexityNodes.first, metaData);
    
    root = resubstitutionErrorBranches(root, metaData, root->rows.size());
    root = nodeNumLeaves(root);
    
    if (minComplexityNodes.second < complexities[i]) output.push_back(std::make_pair(minComplexityNodes.first, -1));
    else {
      while (i <= complexities.size()-1 && minComplexityNodes.second >= complexities[i]) i++;
      output.push_back(std::make_pair(minComplexityNodes.first, i));
    }
  }
  
  return output;
}

Node* costComplexityPrune(Node* node, InputMetaData &metaData, const int &numFoldsCV, const int &maxDepth) {
  
  std::vector<double> updatedComplexities;
  std::vector<std::pair<Node*, double>> treeSeq = complexityPruneTreeSeq(copyNode(node), metaData);
  
  for (int i = 0; i < treeSeq.size()-1; i++) updatedComplexities.push_back(sqrt(treeSeq[i].second*treeSeq[i+1].second));
  
  updatedComplexities.push_back(std::numeric_limits<double>::max());
  
  std::set<int> allRows = node->rows;
  
  std::vector<int> rowsVec(allRows.begin(), allRows.end());
  std::random_shuffle(rowsVec.begin(), rowsVec.end());
  
  int foldSize = std::ceil((double)allRows.size()/(double)numFoldsCV);
  
  Int_Double complexityErrorMap;
  
  for (int i = 1; i <= numFoldsCV; i++) {
    int start = (i-1)*foldSize;
    int end = i*foldSize;
    
    end = (end > rowsVec.size()) ? rowsVec.size():end;
    
    std::set<int> validationRows(rowsVec.begin()+start, rowsVec.begin()+end);
    std::set<int> trainRows;
    
    std::set_difference(allRows.begin(), allRows.end(), validationRows.begin(), validationRows.end(),
                        std::inserter(trainRows, trainRows.begin()));
    
    Node* tree = constructTree(trainRows, metaData, giniImpurity, maxDepth);
    
    std::vector<std::pair<Node*, int>> subTreeSeq = complexityPruneTreeSeqWithComplexities(copyNode(tree), metaData, updatedComplexities);
    
    for (int j = 0; j < subTreeSeq.size(); j++) {
      Node* nodeToPrune = subTreeSeq[j].first;
      
      tree = pruneNode(tree, nodeToPrune, metaData);
      
      if (subTreeSeq[j].second != -1) {
        double error = 0;
        
        for (auto q = validationRows.begin(); q != validationRows.end(); ++q) {
          Int_Double colValMap = metaData.rowColValMap[*q];
          Int_Double predClasses = treePredict(tree, colValMap);
          
          std::pair<int, double> out = bestClass(predClasses);
          
          if (out.first != metaData.rowClassMap[*q]) error++;
        }
        
        error = (double)error/(double)validationRows.size();
        
        complexityErrorMap[subTreeSeq[j].second] += error;
      }
    }
  }
  
  double minError = std::numeric_limits<double>::max();
  int complexityIndex = 0;
  
  for (auto p = complexityErrorMap.begin(); p != complexityErrorMap.end(); ++p) {
    if (p->second < minError) {
      minError = p->second;
      complexityIndex = p->first;
    }
  }
  
  for (int i = 0; i <= complexityIndex; i++) node = pruneNode(node, treeSeq[i].first, metaData);
  
  return node;
}

void foldColValRowsMap(const std::set<int> &featureSet, InputMetaData &metaData) {
  
  for (auto p = featureSet.begin(); p != featureSet.end(); ++p) {
    
    std::set<double> sortedVals = metaData.colSortedVals[*p];
    std::vector<double> sortedValsVec(sortedVals.begin(), sortedVals.end());
    
    for (size_t i = 1; i < sortedValsVec.size(); i++) 
      metaData.colValAllRowsMap[*p][sortedValsVec[i]].insert(metaData.colValAllRowsMap[*p][sortedValsVec[i-1]].begin(), 
                                                             metaData.colValAllRowsMap[*p][sortedValsVec[i-1]].end());
  }
}

// [[Rcpp::export]]
List cpp__adaBoostedTree(DataFrame inputSparseMatrix, std::vector<int> classLabels, 
                         int boostingRounds = 5, int maxDepth=100, int cvRounds=5) {
  
  std::vector<DataFrame> trees;
  std::vector<double> treeWeights;
  
  std::vector<DataFrame> leafNodeClassProbs;
  
  InputMetaData metaData;
  
  std::vector<int> rows = inputSparseMatrix["i"];
  std::vector<int> cols = inputSparseMatrix["j"];
  std::vector<double> vals = inputSparseMatrix["v"];
  
  std::set<int> uniqueRows(rows.begin(), rows.end());
  std::set<int> uniqueCols(cols.begin(), cols.end());
  std::set<int> uniqueLabels(classLabels.begin(), classLabels.end());
  
  metaData.classLabels = uniqueLabels;
  
  for (size_t i = 0; i < rows.size(); i++) metaData.colSortedVals[cols[i]].insert(vals[i]);
  for (size_t i = 0; i < rows.size(); i++) metaData.colValAllRowsMap[cols[i]][vals[i]].insert(rows[i]);
  for (size_t i = 0; i < rows.size(); i++) metaData.rowCols[rows[i]].insert(cols[i]);
  
  for (size_t i = 0; i < rows.size(); i++) metaData.rowColValMap[rows[i]][cols[i]] = vals[i];
  
  for (auto p = uniqueRows.begin(); p != uniqueRows.end(); ++p) metaData.rowClassMap[*p] = classLabels[*p-1];
  
  for (auto p = uniqueRows.begin(); p != uniqueRows.end(); ++p) metaData.instanceWeights[*p]=1/(double)uniqueRows.size();
  
  foldColValRowsMap(uniqueCols, metaData);
  
  int iterNum = 1;

  while(iterNum <= boostingRounds) {

    Node* tree = constructTree(uniqueRows, metaData, giniImpurity, maxDepth);

    tree = costComplexityPrune(tree, metaData, cvRounds, maxDepth);

    double err = 0;

    std::set<int> errorRows;

    for (auto p = uniqueRows.begin(); p != uniqueRows.end(); ++p) {
      std::unordered_map<int, double> predClassProbs = treePredict(tree, metaData.rowColValMap[*p]);
      std::pair<int, double> best = bestClass(predClassProbs);

      if (best.first != metaData.rowClassMap[*p]) {
        errorRows.insert(*p);
        err += metaData.instanceWeights[*p];
      }
    }

    double treeWt;

    if (err > 0.5 || err <= 0) {
      DataFrame predClassProbs;

      trees.push_back(transformTreeIntoDF(tree, predClassProbs));
      treeWeights.push_back(1.0);

      leafNodeClassProbs.push_back(predClassProbs);
      break;
    }
    else {
      treeWt = log((1-err)/err)+log(uniqueLabels.size()-1);

      DataFrame predClassProbs;

      trees.push_back(transformTreeIntoDF(tree, predClassProbs));
      treeWeights.push_back(treeWt);

      leafNodeClassProbs.push_back(predClassProbs);

      double sumWeights = 0;

      for (auto p = uniqueRows.begin(); p != uniqueRows.end(); ++p) {
        if (errorRows.find(*p) != errorRows.end()) metaData.instanceWeights[*p] *= exp(treeWt);
        sumWeights += metaData.instanceWeights[*p];
      }

      for (auto p = uniqueRows.begin(); p != uniqueRows.end(); ++p) metaData.instanceWeights[*p] /= sumWeights;

      iterNum++;
    }
  }

  double sumTreeWeights = 0;

  for (size_t i = 0; i < treeWeights.size(); i++) sumTreeWeights += treeWeights[i];
  for (size_t i = 0; i < treeWeights.size(); i++) treeWeights[i] /= sumTreeWeights;
  
  return List::create(_["Trees"]=trees, _["TreeWeights"]=treeWeights, _["TreeClassProbs"]=leafNodeClassProbs);
}

struct NodeDF {
  int index, leftIndex, rightIndex, featureIndex;
  double decisionVal;
};


std::unordered_map<int, double> dfPredict(std::unordered_map<int, NodeDF> &nodeDFMap,
                                          std::unordered_map<int, double> &colValMap,
                                          std::unordered_map<int, std::unordered_map<int, double>> &predClassProbs) {
  
  int index = 0;
  
  while(true) {
    if (nodeDFMap[index].featureIndex == -1) return predClassProbs[index];
    else {
      if (colValMap[nodeDFMap[index].featureIndex] <= nodeDFMap[index].decisionVal) index = nodeDFMap[index].leftIndex;
      else index = nodeDFMap[index].rightIndex;
    }
  }
}

// [[Rcpp::export]]
std::map<int, std::map<int, double>> cpp__test(DataFrame inputSparseMatrix, List modelsMetaData) {
  
  std::map<int, std::map<int, double>> output;
  
  std::vector<int> rows = inputSparseMatrix["i"];
  std::vector<int> cols = inputSparseMatrix["j"];
  std::vector<double> vals = inputSparseMatrix["v"];
  
  std::unordered_map<int, std::unordered_map<int, double>> rowColValMap;
  
  for (size_t i = 0; i < rows.size(); i++) rowColValMap[rows[i]][cols[i]] = vals[i];
  
  std::vector<DataFrame> models = modelsMetaData["Trees"];
  std::vector<double> modelWeights = modelsMetaData["TreeWeights"];
  std::vector<DataFrame> leafNodeClassProbs = modelsMetaData["TreeClassProbs"];
  
  std::map<int, std::map<int, double>> predProbs;
  
  for (size_t i = 0; i < models.size(); i++) {
    DataFrame model = models[i];
    
    std::vector<int> nodeIndex = model["NodeIndex"];
    std::vector<int> leftNodeIndex = model["LeftNodeIndex"];
    std::vector<int> rightNodeIndex = model["RightNodeIndex"];
    std::vector<int> featureIndex = model["FeatureIndex"];
    std::vector<double> featureDecisionVal = model["FeatureDecisionVal"];
    
    DataFrame leafNodeClassProb = leafNodeClassProbs[i];
    
    std::vector<int> leafIndex = leafNodeClassProb["LeafIndex"];
    std::vector<int> leafLabel = leafNodeClassProb["LeafLabel"];
    std::vector<double> leafLabelProb = leafNodeClassProb["LeafLabelProb"];
    
    std::unordered_map<int, std::unordered_map<int, double>> predClassProbs;
    
    for (size_t j = 0; j < leafIndex.size(); j++) predClassProbs[leafIndex[j]][leafLabel[j]] = leafLabelProb[j];
    
    std::unordered_map<int, NodeDF> nodeDFMap;
    
    for (size_t j = 0; j < nodeIndex.size(); j++) {
      NodeDF df;
      
      df.index = nodeIndex[j];
      df.decisionVal = featureDecisionVal[j];
      df.featureIndex = featureIndex[j];
      df.leftIndex = leftNodeIndex[j];
      df.rightIndex = rightNodeIndex[j];
      
      nodeDFMap[nodeIndex[j]] = df;
    }
    
    for (auto p = rowColValMap.begin(); p != rowColValMap.end(); ++p) {
      std::unordered_map<int, double> out = dfPredict(nodeDFMap, p->second, predClassProbs);
      std::pair<int, double> best = bestClass(out);
      
      for (auto q = out.begin(); q != out.end(); ++q) {
        if (q->first == best.first) predProbs[p->first][q->first] += modelWeights[i];
        else predProbs[p->first][q->first] += 0;
      }
    }
  }
  
  for (auto p = predProbs.begin(); p != predProbs.end(); ++p) {
    std::map<int, double> classProbs = p->second;
    
    output[p->first]=classProbs;
  }
  
  return output;
}