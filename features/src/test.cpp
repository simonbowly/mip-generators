#include "mipfeatures.hpp"

using namespace mipfeatures;
using namespace std;

int main() {

    const auto instance = MIPInstance::readMPS("tests/inst_566700647.mps");
    const auto slow = getLpPolyhedralBounds(instance);
    const auto fast = getLpPolyhedralBoundsFast(instance);

    for (uint i = 0; i < slow.size(); i++) {
        cout
             << fast[i].first
             << " == "
             << slow[i].first
             << " && "
             << fast[i].second
             << " == "
             << slow[i].second
             << endl;
    }

}
