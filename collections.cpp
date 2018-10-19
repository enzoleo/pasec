#include <cmath>
#include <iostream>
#include <iomanip>
#include "collections.hpp"

QueryCollection::QueryCollection(const std::string& filename)
{
  std::ifstream fin;
  fin.open(filename, std::ios::in);
  assert(fin.is_open());
  std::string line;
  while(std::getline(fin, line))
  {
    // The current size of vector @queries.
    std::size_t qn = queries.size();
    queries.emplace_back();
    // The last query is the queries vector.
    Query& query = queries[qn];
    const char delim[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    char* token = std::strtok(&line[0], delim);
    char* otoken = token;
    do
    { // The same steps as the construction of Documents, but we do not
      // have a word dictionary for queries.
      token = std::strtok(NULL, delim);
      std::string word(otoken);
      if (word.length() > 3)
      {
        // Remove ending 's' from the word. Just simply insert the word
        // into query set, that is okay.
        if (word.back() == 's') word.pop_back();
        query.insert(word);
      }
      otoken = token;
    } while (token != NULL);
  }
  fin.close();
}

Document::Document(std::string& str,
                   std::map<std::string, std::size_t>& dict)
{
  // The delimeter to split the string. Here we must remove possible
  // punctuations like ',', '.', '/' etc. But there exists a problem that
  // what should we do if we encounter a word like 'three-year-old'.
  const char delim[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  char* token = std::strtok(&str[0], delim);
  char* otoken = token;
  std::size_t word_count = 0;
  do
  {
    token = std::strtok(NULL, delim);
    std::string word(otoken);
    // If a word has larger than 3 characters and it is not a stopword,
    // remove the ending 's' and register it into the word dictionary.
    if (word.length() > 3 && not_stopword(word))
    {
      // Remove ending 's' from the word.
      if (word.back() == 's') word.pop_back();
      if (!dict.count(word))
      {
        std::size_t cur_size =  dict.size();
        dict[word] = cur_size;
      }
      // Record the position this word occurs in the document.
      if (terms.find(word) == terms.end())
        terms.emplace(std::piecewise_construct,
                      std::forward_as_tuple(word),
                      std::forward_as_tuple());
      terms[word].emplace_back(word_count);
      ++word_count;
    }
    otoken = token;
  } while (token != NULL);
}

std::size_t Document::maxTermVal() const
{
  std::map<std::string, std::size_t> tfvec;
  for (auto &term : this->terms)
    tfvec.insert(std::make_pair(term.first, term.second.size()));

  auto val_comp = [](const TFPair& p1, const TFPair& p2)
                  { return p1.second < p2.second; };
  auto max_iter = std::max_element(tfvec.begin(), tfvec.end(), val_comp);
  return max_iter->second;
}

TextCollection::TextCollection(const std::string& filename)
{
  // Open the text file to load the text collection.
  std::ifstream fin;
  fin.open(filename, std::ios::in);
  assert(fin.is_open());
  for (;;)
  {
    // Read a paragraph and construct a document object.
    std::stringstream ss;
    if (!read_paragraph(fin, ss)) break;
    std::string para = ss.str();
    documents.emplace_back(para, dictionary);
  }
  fin.close();
  this->invDoc();
  this->dwCalc();
  this->l2norms();
}

void TextCollection::invDoc()
{
  this->plists.resize(dictionary.size());
  for (auto &term : this->dictionary)
  {
    std::size_t index = term.second;
    const std::string& str = term.first;
    for (std::size_t j = 0, ndocs = documents.size(); j < ndocs; ++j)
    {
      // If the Document j contains the term i, the posting list of term
      // i @plists[i] should contain a key j which stands for the
      // document j and the corresponding value is the number of its
      // occurrence in this document.
      const Document::TermList& term = documents[j].keys();
      auto iter = term.find(str);
      if (iter != term.end())
        plists[index][j] = iter->second.size();
    }
  }
}

void TextCollection::dwCalc()
{
  // The total number of documents contained in this collection.
  // The following several sentences computes the idf vectors.
  std::size_t ndocs = documents.size();
  std::vector<float> idfvec(plists.size());
  auto idf_calc =
      [&ndocs](const PostingList& plist)
      { return std::log2(1.0 * ndocs / plist.size()); };
  std::transform(plists.begin(), plists.end(), idfvec.begin(), idf_calc);

  // The following @mvals is the tf_max for each document.
  std::vector<float> mvals(documents.size());
  std::transform(documents.begin(), documents.end(), mvals.begin(),
                 [](const Document& doc)
                 { return doc.maxTermVal(); });
  
  weights.resize(documents.size());
  for (std::size_t i = 0, ndocs = documents.size(); i < ndocs; ++i)
    for (auto &term : documents[i].keys())
    {
      const std::string& word = term.first;
      std::size_t tf = term.second.size();
      float weight = tf * idfvec[dictionary[word]] / mvals[i];
      weights[i].emplace(word, weight);
    }
}

void TextCollection::l2norms()
{
  std::size_t ndocs = documents.size();
  dnorms.resize(ndocs);
  for (std::size_t i = 0; i < ndocs; ++i)
  {
    const DocWeights& dw = weights[i];
    float sum = 0;
    for (auto &term : dw) sum += std::pow(term.second, 2);
    dnorms[i] = std::sqrt(sum);
  }
}

void TextCollection::similarity(
    const Query& query,
    std::map<std::size_t, float>& sim) const
{
  float qnorm = std::sqrt(query.size());
  for (auto &word : query)
  {
    // Get the index and the posting list of this word.
    // Note that a word in the query is possible to not appear in the word
    // dictionary of this text collection.
    auto iter = dictionary.find(word);
    // If a word does not appear in the word dictionary, it contributes
    // nothing to the weight vector.
    if (iter == dictionary.end()) continue;
    std::size_t index = iter->second;
    const TextCollection::PostingList& plist = plists[index];
    for (auto &posting: plist)
    {
      // Here we don't check whether the std::map::find function returns
      // a correct iterator which means that it must not return the last
      // iterator std::map::end().
      std::size_t did = posting.first;
      auto iter = weights[did].find(word);
      float weight = iter->second / (dnorms[did] * qnorm);
      auto endit = sim.end();
      if (sim.find(did) != endit) sim[did] += weight;
      else sim[did] = weight;
    }
  }
}

void TextCollection::docInfo(std::size_t did, std::size_t nkws) const
{
  std::cout << "DID: " << did << std::endl;
  // Get the five highest weighted keywords of the document.
  const TextCollection::DocWeights& dw = weights[did];
  std::vector<std::pair<std::string, float> >
      pairs(dw.begin(), dw.end());
  std::partial_sort(pairs.begin(), pairs.begin() + nkws, pairs.end(),
                    [](const std::pair<std::string, float>& p1,
                       const std::pair<std::string, float>& p2)
                    { return p1.second > p2.second; });
  // Get the corresponding posting list.
  for (auto it = pairs.begin(), end = pairs.begin() + 5;
       it != end; ++it)
  {
    const std::string& word = it->first;
    // Find the index of this word in the word dictionary. Usually it will
    // find unique index because the word appears in the document should
    // be in the dictionary. Therefore the check for iter is not necessary.
    // However we still do this here for accidental cases (Better not
    // occurs).
    auto iter = dictionary.find(word);
    if (iter == dictionary.end()) continue;

    // Obtain the posting list.
    const std::size_t index = iter->second;
    const TextCollection::PostingList& plist = plists[index];
    std::cout << std::left << std::setw(14) << word << " -> | ";
    for (auto &posting : plist)
    {
      // Get the document ID and the term of the word
      const std::size_t docID = posting.first;
      const Document::TermList terms = documents[docID].keys();
      auto iter = terms.find(word);
      if (iter == terms.end()) continue;

      // Get the position vector. 
      const std::vector<std::size_t>& vec = iter->second;
      std::cout << "D" << docID << ":" << vec[0];
      for (std::size_t i = 1, end = vec.size(); i < end; ++i)
        std::cout << "," << vec[i];
      std::cout << " | ";
    }
    std::cout << std::endl;
  }
  // Print other information required by the assignment on the screem, such
  // as the number of unique keywords in the document and the magnitude of
  // the weight vector of this document.
  std::cout << "Number of unique keywords in document: "
            << documents[did].keys().size() << std::endl;
  std::cout << "Magnitude of the document vector (L2 norm): "
            << dnorms[did] << std::endl;
}

void TextCollection::search(const QueryCollection& qc,
                            std::size_t nsorting,
                            std::size_t nkws) const
{
  const std::string outer_hrule = std::string(60, '=') + "\n";
  const std::string inner_hrule = std::string(50, '-') + "\n";
  for (auto &query : qc.queries)
  {
    // For each query compute the similarity scores.
    std::cout << outer_hrule;
    std::map<std::size_t, float> sim;
    this->similarity(query, sim);

    // Sort the similarity score if sorting flag is true.
    using ScorePair = std::pair<std::size_t, float>;
    std::vector<ScorePair> simvec(sim.begin(), sim.end());
    if (nsorting)
      std::partial_sort(
          simvec.begin(), simvec.begin() + nsorting, simvec.end(),
          [](const ScorePair& sp1, const ScorePair& sp2)
          { return sp1.second > sp2.second; });

    // Output the result.
    std::cout << "Query: ";
    for (auto &word : query) std::cout << word << " ";
    std::cout << std::endl;
    for (auto it = simvec.begin(), end = simvec.begin() + nsorting;
         it != end; ++it)
    { // Print the document information on screen.
      float score = it->second;
      std::size_t did = it->first;
      this->docInfo(did, nkws);
      std::cout << "Similarity score: " << score << std::endl;
      if (it != end - 1)
        std::cout << inner_hrule;
    }
  }
  std::cout << outer_hrule;
}


