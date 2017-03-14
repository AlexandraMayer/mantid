#include "MantidDataHandling/SaveYDA.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include <fstream>
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveYDA)
using namespace API;
using namespace Kernel;

SaveYDA::SaveYDA() {}

/// Initialisation method.
void SaveYDA::init() {

    auto wsValidator = boost::make_shared<CompositeValidator>();
    wsValidator->add<WorkspaceUnitValidator>("DeltaE");
    wsValidator->add<InstrumentValidator>();

    //declare mandatory prpperties
    declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                        "InputWorkspace", "" , Direction::Input, wsValidator) ,
                    "The workspace name to use as Input");


    declareProperty(make_unique<FileProperty>("Filename", "",
                         FileProperty::Save, ""),
                    "The name to use when writing the file");

}

void getBinCenters(Axis *axis, std::vector<double>& result) {
    for(size_t i = 1; i < axis->length(); i++) {
        result.push_back((axis->getValue(i) + axis->getValue(i-1))/2);
   }
}

void SaveYDA::exec() {

    const std::string filename = getProperty("Filename");
    g_log.debug() << "SaveYDA filename = " << filename << std::endl;

    const MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

    /*
    g_log.debug() << "Workspace =" << ws->getName() << std::endl;
    auto &yValue = ws->y(0);
    g_log.debug() << "Y-Val at 0 = " << yValue[0] << std::endl;
    auto &xValue = ws->x(0);
    g_log.debug() << "X-Val at 0 = " << xValue[0] << std::endl;
    g_log.debug() << "T = " << temperature << std::endl;
    g_log.debug() << "Ei = " << ei << std::endl;
    g_log.debug() << "proposal_number = " << proposal_numberd << std::endl;
    g_log.debug() << "experiment_team = " << (experiment_team) << std::endl;
    g_log.debug() << "proposal_title = " << proposal_title << std::endl;
    std::string x_unit = X->unit()->unitID();
    g_log.debug() << " x title " << X->unit()->caption()<< " x unit "  << x_unit  <<  std::endl;
    std::string y_unit = Z->unit()->unitID();
    g_log.debug() << "y title" << Z->unit()->caption() << " y unit " <<y_unit << std::endl;
    g_log.debug() << "length: " << Z->length() << std::endl;
    for(int i= 0; i < Z->length()-1; i++) {
        g_log.debug() << "j: " <<  i << "\n z: " << (ws->getAxis(1)->getValue(i)) << "x: " << ws->x(i)[0] << "y: " << ws->y(i)[0] << std::endl;
    }

*/
    //open file for writing
    std::ofstream fout(filename.c_str());

    if(!fout) {
        g_log.error("Failed to open file: " + filename);
        throw Exception::FileError("Failed to open file ", filename);
    }

    //initializing metadata
    metadata["format"] = "yaml/frida 2.0";
    metadata["type"] = "generic tabular data";
    YAML::Emitter em;

    auto ld = ws->run().getLogData();

    if(ld.size() == 0) {
        g_log.error("No sample log data");
        throw Exception::ExistsError("No sample log data exists in ", filename);
    }

    if(ws->run().hasProperty("proposal_number")) {
        std::string writing = "propsal number: " ;
        std::string proposal_number = std::to_string((int)(ws->run().getLogAsSingleValue("proposal_number")));
        history.push_back(writing + proposal_number);
    } else {
        g_log.warning("no proposal number found");
    }

    if(ws->run().hasProperty("proposal_title")) {
        auto  proposal_title = ws->run().getLogData("proposal_title")->value();
        history.push_back(proposal_title);
    } else {
        g_log.warning("no proposal title found");
    }

    if(ws->run().hasProperty("experiment_team")) {
        auto experiment_team = ws->run().getLogData("experiment_team")->value();
        history.push_back(experiment_team);
    } else {
        g_log.warning("no experiment team found");
    }

    history.push_back("data reduced with mantid");


    if(!(ws->run().hasProperty("temperature"))) {
        g_log.warning("no temperature found");
    } else {
        double temperature = ws->run().getLogAsSingleValue("temperature");

        ParStruct temppar;
        temppar.name = "T";
        temppar.unit = "K";
        temppar.value = temperature;
        temppar.stdv = 0;

        rpar.push_back(temppar);
    }


    if(!(ws->run().hasProperty("Ei")) ) {
        g_log.warning("no Ei found");
    } else {
        double ei = ws->run().getLogAsSingleValue("Ei");

        ParStruct eipar;
        eipar.name = "Ei";
        eipar.unit = "meV";
        eipar.value = ei;
        eipar.stdv = 0;

        rpar.push_back(eipar);
    }



    Axis *X = ws->getAxis(0);

    if(X != nullptr)
       std::cout <<  "not a nullpointer" << std::endl;
    Coordinate xc;
    xc.designation = "x";
    if(X->isSpectra()) {
        xc.name = "2th";
        xc.unit = "deg";
    } else {
        xc.name = X->unit()->caption();
        if(xc.name == "q") {
            xc.unit= "A-1";
        } else{
            xc.unit = X->unit()->unitID();
        }
        if(xc.unit == "DeltaE")
            xc.unit = "w";
    }

    coord.push_back(xc);

    Coordinate yc;
    yc.designation = "y";
    yc.name = "S(q,w)";
    yc.unit = "meV-1";

    coord.push_back(yc);

    Axis *Z = ws->getAxis(1);

    Coordinate zc;
    zc.designation = "z";
    if(Z->isSpectra()) {
        zc.name = "2th";
        zc.unit = "deg";
    } else {
        zc.name = Z->unit()->caption();
        if(zc.name == "q") {
            zc.unit= "A-1";
        } else {
            zc.unit = Z->unit()->unitID();
         }
        if(zc.unit == "DeltaE")
            zc.unit = "w";
    }

    coord.push_back(zc);

    std::vector<double> bin_centers(4,0.12);

    getBinCenters(ws->getAxis(1), bin_centers);



    for(unsigned int i = 0; i < Z->length()-1; i++) {
        auto xs = ws->x(i);
        auto ys = ws->y(i);
        //std::vector<double> x;
        std::vector<double> y;
        std::vector<double> x_centers;
        getBinCenters(X, x_centers);
       // for(unsigned int j = 0; j < xs.size(); j++) {
        //    x.push_back(xs[j]);
       // }
        for(unsigned int k = 0; k < ys.size(); k++) {
            y.push_back(ys[k]);
        }
        slices.push_back(Spectrum(i,bin_centers[i],x_centers,y));
    }


    writeHeader(em);
    fout << em.c_str();
    fout.close();


}



YAML::Emitter& operator << (YAML::Emitter& em,const Coordinate c) {
    em << YAML::BeginMap << YAML::Key << c.designation << YAML::Flow << YAML::BeginMap
       << YAML::Key << "name" << YAML::Value << c.name << YAML::Key << "unit"
       << YAML::Value << c.unit << YAML::EndMap << YAML::EndMap;
    return em;
}

YAML::Emitter& operator << (YAML::Emitter& em,const ParStruct p) {
    em << YAML::BeginMap << YAML::Key << "name" << YAML::Value << p.name
        << YAML::Key << "unit" << YAML::Key << p.unit << YAML::Key << "val"
        << YAML::Value << p.value << YAML::Key << "stdv" << YAML::Value << p.stdv
        << YAML::EndMap;
    return em;
}

YAML::Emitter& operator << (YAML::Emitter& em,const Spectrum s) {
    em << YAML::BeginMap << YAML::Key << "j" << YAML::Value << s.j
       << YAML::Key << "z" << YAML::Value << YAML::Flow << YAML::BeginSeq
       << YAML::Flow << YAML::BeginMap << YAML::Key << "val"
       << YAML::Value << s.z << YAML::EndMap << YAML::EndSeq
       << YAML::Key << "x" << YAML::Value << YAML::Flow << s.x
       << YAML::Key << "y" << YAML::Value << YAML::Flow << s.y
       << YAML::EndMap;
    return em;
}

void SaveYDA::writeHeader(YAML::Emitter& em) {
    em << YAML::BeginMap << YAML::Key << "Meta";
    em << YAML::Value << metadata;
    em << YAML::Key << "History" << YAML::Value << history;
    em << YAML::Key << "RPar" <<  rpar ;
    em << YAML::Key << "Coord" << coord;
    em << YAML::Key << "Slices" << slices <<YAML::EndMap;
}

}
}
