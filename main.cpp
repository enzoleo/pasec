#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>
#include <map>
#include <iomanip>
#include "utils.hpp"
#include "collections.hpp"

int main(int argc, char** argv)
{
  std::string tcf, qcf;
  switch (argc)
  {
    case 1:
      tcf = "collection-100.txt";
      qcf = "query-10.txt";
      break;
    case 3:
      tcf = argv[1];
      qcf = argv[2];
      break;
    default:
      std::cerr << "Invalid arguments." << std::endl;
      return 1;
  }
  
  TextCollection tc(tcf);
  QueryCollection qc(qcf);
  search(tc, qc, 3, 5);
  return 0;
}

