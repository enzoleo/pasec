#ifndef COLLECTION_HPP_
#define COLLECTION_HPP_

#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <cctype>
#include "utils.hpp"

using Query = std::set<std::string>;
struct QueryCollection
{
  /**
   * The constructor with no argument is deleted. You must specify a
   * filename explicitly to construct an instance of this class.
   */
  constexpr QueryCollection() = delete;
  QueryCollection(const std::string& filename);
      
  std::vector<Query> queries;
};

class Document
{
public:
  /**
   * TermList is alias for a position dictionary. For a document, each word
   * that has appeared is saved as a key in the dictionary @terms, and
   * all the positions of it are saved in a vector as the value of this key.
   *
   * TFPair is alias for term frequency. A pair of word with its frequency
   * in the document is considered as a TFPair.
   */
  using TermList = std::map <std::string, std::vector<std::size_t> >;
  using TFPair   = std::pair<std::string, std::size_t>;
  
  Document() = default;
  /**
   * @brief Constructor with a paragraph raw string.
   * @param str the raw paragraph string which contains each possible word
   *        as a substring splited by delimeters.
   *        dict the word dictionary.
   *
   * Each word extracted out will be saved as a key in this dictionary, and
   * an index will be automatically generated as the value of this key, as
   * long as the word is valid.
   */
  Document(std::string& str,
           std::map<std::string, std::size_t>& dict);

  std::size_t maxTermVal() const;
  const TermList& keys() const { return this->terms; }

private:

  TermList terms;
};

class TextCollection
{
public:
  using DocWeights  = std::map<std::string, float>;
  using PostingList = std::map<std::size_t, std::size_t>;
  using Dictionary = std::map<std::string, std::size_t>;

  /**
   * The constructor with no argument is deleted, so you must specify a
   * filename of a text file containing the collection data to instantiate
   * the class.
   */
  constexpr TextCollection() = delete;
  TextCollection(const std::string& filename);

  /**
   * Accessors of the private attributes.
   * Note that this class is extremely stable, which means you are not
   * allowed to do modification on the attributes by yourself.
   * Considering that it will cost too much to reconstruct the class object
   * once you append a new document into this collection, the user is
   * definitely not allowed to directly access the doc vector and modify it.
   */
  const std::vector<Document>& docs()  const { return this->documents;  }
  const Dictionary& dict()             const { return this->dictionary; }
  const std::vector<PostingList>& pl() const { return this->plists;     }
  const std::vector<DocWeights>&  wv() const { return this->weights;    }
  const std::vector<float>&    norms() const { return this->dnorms;     }

  void search(const QueryCollection& qc,
              std::size_t nsorting,
              std::size_t nkws) const;

  void docInfo(std::size_t did, std::size_t nkws) const;

  void similarity(const Query& query,
                  std::map<std::size_t, float>& sim) const;
  
private:
  void invDoc();
  void dwCalc();
  void l2norms();
  
  std::vector<Document> documents;
  Dictionary dictionary;
  
  std::vector<PostingList> plists;
  std::vector<DocWeights> weights;
  std::vector<float> dnorms;
};

inline void search(
    const TextCollection& tc,
    const QueryCollection& qc,
    std::size_t nsorting = 3,
    std::size_t nkws = 5)
{
  return tc.search(qc, nsorting, nkws);
}

#endif

