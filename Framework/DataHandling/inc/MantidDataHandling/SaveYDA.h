#ifndef MANTID_DATAHANDLING_SAVEYDA_H_
#define MANTID_DATAHANDLING_SAVEYDA_H_

//#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/TextAxis.h"
#include <yaml-cpp/yaml.h>


namespace Mantid {
namespace DataHandling {

/** SaveYDA : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

struct ParStruct {
    std::string name;
    std::string unit;
    double value;
    double stdv;
};

struct Coordinate {
    std::string name;
    std::string unit;
    std::string designation;
};

struct Spectrum {
    int j;
    double z;
    std::vector<double> x;
    std::vector<double> y;

    Spectrum(int n_j, double n_z, std::vector<double> n_x, std::vector<double> n_y) {
        j = n_j; z = n_z; x = n_x; y = n_y;
    }
};

YAML::Emitter& operator << (YAML::Emitter& em,const ParStruct p);
YAML::Emitter& operator << (YAML::Emitter& em,const Coordinate c);
YAML::Emitter& operator << (YAML::Emitter& em,const Spectrum s);
using namespace API;

class DLLExport SaveYDA : public API::Algorithm {
public:



    SaveYDA();

    const::std::string name () const override { return "SaveYDA"; }
    int version() const override { return 1; }
    const std::string summary() const override { return "Saves a 2D workspace to a FRIDA yaml file."; }

private:
    void init() override;
    virtual void exec() override;
    //std::map<std::string, std::string> getMetadata() {return metadata;}
    void setMetadata(std::map<std::string, std::string> nMetadata) { metadata = nMetadata;}
    void writeHeader(YAML::Emitter& em);
    void getBinCenters(Axis *axis, std::vector<double> &result);
    //void setRpar(std::vector<ParStruct>  nRpar) { rpar = nRpar;}
    //std::vector<ParStruct>  getRpar() {return rpar;}
    //std::vector<std::string> getHistory() { return history; }

    std::map<std::string, std::string> metadata;
    std::vector<ParStruct> rpar;
    std::vector<std::string> history;
    std::vector<Coordinate> coord;
    std::vector<Spectrum> slices;


};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEYDA_H_ */
