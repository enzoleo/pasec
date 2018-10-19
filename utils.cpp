#include "utils.hpp"

bool not_stopword(const std::string& word)
{
  for (auto &stopword : stopwords)
    if (!stopword.compare(word)) return false;
  return true;
}

bool read_paragraph(std::istream& fin, std::stringstream& ss)
{
  std::string line;
  do
  { // Read each line into the stringstream until encountering an empty
    // line, which means the paragraph ends here.
    std::getline(fin, line);
    ss << line;
  } while (line != "");
  
  // If the string is empty, this function has not read a paragraph and
  // it will directly return false.
  if (ss.str().empty()) return false;
  return true;
}

